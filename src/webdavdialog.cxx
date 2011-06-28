#include "webdavdialog.hxx"
#include <osl/diagnose.h>
#include <rtl/ustring.hxx>
#include <cppuhelper/implbase1.hxx>
#include <com/sun/star/awt/Key.hpp>
#include <com/sun/star/awt/PosSize.hpp>
#include <com/sun/star/awt/WindowAttribute.hpp>
#include <com/sun/star/awt/XButton.hpp>
#include <com/sun/star/awt/XControl.hpp>
#include <com/sun/star/awt/XControlContainer.hpp>
#include <com/sun/star/awt/XControlModel.hpp>
#include <com/sun/star/awt/XDialog.hpp>
#include <com/sun/star/awt/XDialogProvider2.hpp>
#include <com/sun/star/awt/XWindowPeer.hpp>
#include <com/sun/star/awt/XMessageBox.hpp>
#include <com/sun/star/awt/XItemList.hpp>
#include <com/sun/star/awt/XListBox.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/deployment/PackageInformationProvider.hpp>
#include <com/sun/star/deployment/XPackageInformationProvider.hpp>
#include <com/sun/star/frame/FrameSearchFlag.hpp>
#include <com/sun/star/frame/XComponentLoader.hpp>
#include <com/sun/star/frame/XController.hpp>
#include <com/sun/star/frame/XStorable.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/ucb/XSimpleFileAccess.hpp>

#include <cstdio> // TEMPORARY: for puts

using rtl::OUString;
using namespace css::awt;
using namespace css::beans;
using namespace css::container;
using namespace css::deployment;
using namespace css::frame;
using namespace css::lang;
using namespace css::uno;
using css::lang::XMultiComponentFactory;
using css::awt::Key::RETURN;
using css::awt::PosSize::POSSIZE;

/* Action listener */
class WebDAVDialogActionListener : public ::cppu::WeakImplHelper1< css::awt::XActionListener >
{
private:
    WebDAVDialog * const owner;

public:
    WebDAVDialogActionListener (WebDAVDialog * const _owner)
       : owner (_owner) { }

    // XEventListener
    virtual void SAL_CALL disposing (const css::lang::EventObject &aEventObj) throw (css::uno::RuntimeException)
    {
        puts ("dispose action listener");
    }

    // XActionListener
    virtual void SAL_CALL actionPerformed (const css::awt::ActionEvent &rEvent) throw (css::uno::RuntimeException)
    {
        /* Obtain the name of the control the event originated from */
        Reference< XControl > control (rEvent.Source, UNO_QUERY);
        Reference< XControlModel > controlModel = control->getModel ();
        Reference< XPropertySet > controlProps (controlModel, UNO_QUERY);
        css::uno::Any aValue = controlProps->getPropertyValue (OUString::createFromAscii ("Name"));
        OUString controlName;
        aValue >>= controlName;
        printf ("action performed: %s\n", OUStringToOString (controlName, RTL_TEXTENCODING_UTF8).getStr ());

        if (controlName.equalsAscii ("OpenButton"))
        {
            owner->openOrSaveSelectedDocument ();
        }
        else if (controlName.equalsAscii ("FileList"))
        {
            /* This will work fine, because the first click will
             * select the item.  Which means the item will be
             * selected when we enter this method.
             */
            owner->openOrSaveSelectedDocument ();
        }
        else if (controlName.equalsAscii ("OpenLocationButton"))
        {
            /* FIXME: Is this okay with regard to threading, etc. ? */
            owner->dumpDAVListing ();
        }
        else if (controlName.equalsAscii ("CancelButton"))
        {
            owner->closeDialog ();
        }
    }
};

/* Key listener */
class WebDAVDialogKeyListener : public ::cppu::WeakImplHelper1< css::awt::XKeyListener >
{
private:
    WebDAVDialog * const owner;

public:
    WebDAVDialogKeyListener (WebDAVDialog * const _owner)
       : owner (_owner) { }

    // XEventListener
    virtual void SAL_CALL disposing (const css::lang::EventObject &aEventObj) throw (css::uno::RuntimeException)
    {
        puts ("dispose key listener");
    }

    // XKeyListener
    virtual void SAL_CALL keyPressed (const css::awt::KeyEvent &rEvent) throw (css::uno::RuntimeException)
    {
        /* Obtain the name of the control the event originated from */
        Reference< XControl > control (rEvent.Source, UNO_QUERY);
        Reference< XControlModel > controlModel = control->getModel ();
        Reference< XPropertySet > controlProps (controlModel, UNO_QUERY);
        css::uno::Any aValue = controlProps->getPropertyValue (OUString::createFromAscii ("Name"));
        OUString controlName;
        aValue >>= controlName;

        puts ("key pressed");

        if (controlName.equalsAscii ("LocationEntry"))
        {
            short aKey = rEvent.KeyCode;

            puts("LocationEntry key pressed");

            if (aKey == RETURN)
            {
                owner->dumpDAVListing ();
            }
        }
    }

    virtual void SAL_CALL keyReleased (const css::awt::KeyEvent &rEvent) throw (css::uno::RuntimeException)
    {
        puts ("key released");
    }
};


/* Dialog construction */

WebDAVDialog::WebDAVDialog( const Reference< css::uno::XComponentContext > &rxContext,
                            const Reference< css::frame::XFrame >          &rxFrame,
                            const sal_Bool                                  isSave) : mxContext ( rxContext ),
                                                                                      mxFrame ( rxFrame ),
                                                                                      isSave ( isSave)
{
    puts ("dialog ctor");

    mxMCF = mxContext->getServiceManager ();

    // Create the toolkit to have access to it later
    mxToolkit = Reference< XToolkit >( mxMCF->createInstanceWithContext(
                                        OUString( RTL_CONSTASCII_USTRINGPARAM(
                                            "com.sun.star.awt.Toolkit" )), mxContext), UNO_QUERY );

    createDialog ();
}

void WebDAVDialog::createDialog (void)
{
    /* Construct path to XDL file in extension package */
    Reference< XPackageInformationProvider> infoProvider =
        PackageInformationProvider::get (mxContext);

    OUString dialogFile(RTL_CONSTASCII_USTRINGPARAM("/open.xdl"));
    OUString packageUrl(infoProvider->getPackageLocation(OUString::createFromAscii("com.lanedo.webdavui")));
    if (packageUrl.getLength() == 0)
        packageUrl = OUString::createFromAscii("file:///usr/lib/libreoffice/share/extensions/webdavui");
    OUString dialogUrl(packageUrl + dialogFile);
    printf ("Loading UI from %s...\n",
            OUStringToOString (dialogUrl, RTL_TEXTENCODING_UTF8).getStr ());

    /* Create dialog from file */
    Reference< XInterface > dialogProvider =
        mxMCF->createInstanceWithContext(OUString::createFromAscii("com.sun.star.awt.DialogProvider2"), mxContext);
    Reference< XDialogProvider2 > dialogProvider2(dialogProvider, UNO_QUERY);
    dialog = dialogProvider2->createDialog(dialogUrl);
    if (!dialog.is())
    {
        printf ("Failed to load dialog, bailing out\n");
        return;
    }

    Reference< XDialog > realDialog (dialog, UNO_QUERY);

    /* FIXME, these strings need to be translatable */
    if (isSaveDialog ())
    {
        realDialog->setTitle(OUString::createFromAscii("Save a File To the Cloud"));
    }
    else
    {
        realDialog->setTitle(OUString::createFromAscii("Open a File From the Cloud"));
    }

    /* Put the dialog in a window */
    Reference< XControl > control(dialog, UNO_QUERY);
    Reference< XWindow > window(control, UNO_QUERY);
    window->setVisible(true);
    control->createPeer(mxToolkit,NULL);

    /* Get the open/save button */
    Reference< XControlContainer > controlContainer (dialog, UNO_QUERY);
    Reference< XControl > openButton =
        controlContainer->getControl (OUString::createFromAscii ("OpenButton"));
    Reference< XControlModel > openButtonModel =
        openButton->getModel ();

    Reference< XPropertySet > openProps (openButtonModel, UNO_QUERY);

    /* FIXME, these strings need to be translatable */
    if (isSaveDialog ())
    {
      openProps->setPropertyValue(OUString::createFromAscii("Label"),
                                  makeAny (OUString::createFromAscii("Save Document")));
    }
    else
    {
      openProps->setPropertyValue(OUString::createFromAscii("Label"),
                                  makeAny (OUString::createFromAscii("Open Document")));
    }

    /* Create event listeners */
    Reference< XActionListener > actionListener =
        static_cast< XActionListener *> (new WebDAVDialogActionListener (this));

    Reference< XKeyListener > keyListener =
        static_cast< XKeyListener *> (new WebDAVDialogKeyListener (this));

    Reference< XButton > openButtonControl (openButton, UNO_QUERY);
    openButtonControl->addActionListener (actionListener);

    /* Connect List URL button to action listener */
    Reference< XInterface > buttonObject =
        controlContainer->getControl (OUString::createFromAscii ("OpenLocationButton"));
    Reference< XButton > buttonControl (buttonObject, UNO_QUERY);

    buttonControl->addActionListener (actionListener);

    /* Connect cancel button to action listener */
    Reference< XInterface > cancelObject =
        controlContainer->getControl (OUString::createFromAscii ("CancelButton"));
    Reference< XButton > cancelControl (cancelObject, UNO_QUERY);

    cancelControl->addActionListener (actionListener);

    Reference< XControl > entryControl =
        controlContainer->getControl (OUString::createFromAscii ("FileEntry"));
    fileEntryModel = entryControl->getModel ();
    Reference< XPropertySet > entryProps (fileEntryModel, UNO_QUERY);
    if (isSaveDialog ())
    {
        Reference< XController > xController = mxFrame->getController();
        if ( !xController.is() )
            return;
        Reference< XModel > xModel (xController->getModel());
        Reference< XStorable > xStorable( xModel, UNO_QUERY );
        if (!xStorable.is())
            return;

        if (xStorable->hasLocation ())
        {
            OUString fileName (xStorable->getLocation().copy (
                xStorable->getLocation().lastIndexOf (OUString::createFromAscii ("/")) + 1));
            entryProps->setPropertyValue(OUString::createFromAscii("Text"), makeAny (fileName));
        }
    }

    /* Save references to the file list and location entry models */
    Reference< XControl > listControl =
        controlContainer->getControl (OUString::createFromAscii ("FileList"));
    fileListModel = listControl->getModel ();

    OUString remoteServer (OUString::createFromAscii("http://localhost/dav/"));

    /* Connect the entry to a key listener and get its model */
    entryControl =
        controlContainer->getControl (OUString::createFromAscii ("LocationEntry"));
    locationEntryModel = entryControl->getModel ();
    entryProps = Reference< XPropertySet> (locationEntryModel, UNO_QUERY);
    entryProps->setPropertyValue(OUString::createFromAscii("Text"), makeAny (remoteServer));

    Reference< XControl > labelControl =
        controlContainer->getControl (OUString::createFromAscii ("Location"));
    Reference< XInterface > locationLabelModel (labelControl->getModel ());
    Reference< XPropertySet > labelProps (locationLabelModel, UNO_QUERY);
    labelProps->setPropertyValue(OUString::createFromAscii("Label"), makeAny (remoteServer));

    Reference< XWindow > entryWindow (entryControl, UNO_QUERY);
    entryWindow->addKeyListener (keyListener);

    /* Connect the list box to an action listener */
    Reference< XListBox > listBox(listControl, UNO_QUERY);
    listBox->addActionListener (actionListener);

    /* Put a placeholder item in the list box */
    Reference< XPropertySet > listProps (fileListModel, UNO_QUERY);
    Sequence < OUString > entries (1);
    entries[0] = OUString::createFromAscii ("(content listing will appear here)");

    listProps->setPropertyValue(OUString::createFromAscii("StringItemList"), makeAny (entries));


    /* The grid control */
    gridModel =
      mxMCF->createInstanceWithContext(OUString::createFromAscii("com.sun.star.awt.grid.UnoControlGridModel"),
                                       mxContext);

    if (!gridModel.is())
      {
        puts("EEEEEEEEEEEEEEK");
        return;
      }

    Reference< XControlModel > model = control->getModel();
    Reference< XNameContainer > nameContainer (model, UNO_QUERY);

     if (!nameContainer.is())
      {
        puts("EEEEEEEEEEEEEEK name container");
        return;
      }

     nameContainer->insertByName (OUString::createFromAscii ("FileGrid"),
                                  makeAny (gridModel));

     Reference< XControl > gridControl =
       controlContainer->getControl (OUString::createFromAscii ("FileGrid"));
     Reference< XWindow > gridWindow (gridControl, UNO_QUERY);

     gridWindow->setPosSize (7, 400, 400, 200, POSSIZE);
     gridWindow->setVisible (true);
}

sal_Bool WebDAVDialog::isSaveDialog (void)
{
    return isSave;
}

void WebDAVDialog::show (void)
{
    /* Execute the clear */
    Reference< XDialog > xDialog(dialog,UNO_QUERY);
    xDialog->execute();

    /* After execution: get the XComponent interface of the dialog
     * and dispose the dialog.
     */
    Reference< XComponent > xComponent(dialog,UNO_QUERY);
    xComponent->dispose();
}

void WebDAVDialog::closeDialog (void)
{
    Reference< XDialog > xDialog(dialog,UNO_QUERY);
    xDialog->endExecute();
}

void WebDAVDialog::showMessageBox (void)
{
    if ( ! (mxFrame.is() && mxToolkit.is()) )
        return;

    // describe window properties.
    WindowDescriptor                aDescriptor;
    aDescriptor.Type              = WindowClass_MODALTOP;
    aDescriptor.WindowServiceName = OUString( RTL_CONSTASCII_USTRINGPARAM( "infobox" ));
    aDescriptor.ParentIndex       = -1;
    aDescriptor.Parent            = Reference< XWindowPeer >( mxFrame->getContainerWindow(), UNO_QUERY );
    aDescriptor.Bounds            = Rectangle(0,0,300,200);
    aDescriptor.WindowAttributes  = WindowAttribute::BORDER |
        WindowAttribute::MOVEABLE |
        WindowAttribute::CLOSEABLE;

    Reference< XWindowPeer > xPeer = mxToolkit->createWindow( aDescriptor );

    if ( xPeer.is() )
    {
        Reference< XMessageBox > xMsgBox( xPeer, UNO_QUERY );
        if ( xMsgBox.is() )
        {
            xMsgBox->setCaptionText( OUString( RTL_CONSTASCII_USTRINGPARAM( "This is my caption" )));
            xMsgBox->setMessageText( OUString( RTL_CONSTASCII_USTRINGPARAM( "This is my message" )));
            xMsgBox->execute();
        }
    }
}

void WebDAVDialog::openOrSaveSelectedDocument (void)
{
    if ( ! (mxFrame.is() && mxToolkit.is()) )
        return;

    Reference< XPropertySet > entryProps (fileListModel, UNO_QUERY);
    Any aValue = entryProps->getPropertyValue (OUString::createFromAscii ("SelectedItems"));
    Sequence< short > selectedItems;
    aValue >>= selectedItems;

    sal_Int32 n = selectedItems.getLength ();
    for (sal_Int32 i = 0; i < n; i++)
    {
        const Reference< XItemList > items(fileListModel, UNO_QUERY_THROW );
        Any aURL = items->getItemData(selectedItems[i]);
        OUString sURL;
        aURL >>= sURL;
        /* No URL, so this was informational or an error message */
        if ( sURL.getLength() == 0 )
            return;

        Reference< XComponentLoader > xLoader(mxFrame, UNO_QUERY);
        Sequence< PropertyValue > lProperties (1);

        /* Set interaction handler */
        Reference< css::task::XInteractionHandler > interactionHandler =
            Reference< css::task::XInteractionHandler > (
                    mxMCF->createInstanceWithContext (OUString::createFromAscii ("com.sun.star.task.InteractionHandler"),
                                                      mxContext), UNO_QUERY);
        lProperties[0].Name = OUString::createFromAscii ("InteractionHandler");
        lProperties[0].Value = makeAny (interactionHandler);

        if (isSaveDialog ())
        {
            printf ("Saving document: %s\n",
                    OUStringToOString (sURL, RTL_TEXTENCODING_UTF8).getStr ());

            Reference< XController > xController = mxFrame->getController();
            if ( !xController.is() )
                return;
            Reference< XModel > xModel (xController->getModel());
            Reference< XStorable > xStorable( xModel, UNO_QUERY );
            if (!xStorable.is())
                return;

            xStorable->storeAsURL(sURL, lProperties);
            /* Saving multiple documents makes no sense */
            break;
        }
        else
        {
            printf ("Opening document: %s\n",
                    OUStringToOString (sURL, RTL_TEXTENCODING_UTF8).getStr ());
            Reference< XComponent > xDocument (xLoader->loadComponentFromURL(
                sURL, mxFrame->getName(), FrameSearchFlag::CHILDREN, lProperties));
        }
    }

    /* Close dialog when done */
    closeDialog ();
}

void WebDAVDialog::dumpDAVListing (void)
{
    /* Get text from our location entry */
    Reference< XPropertySet > entryProps (locationEntryModel, UNO_QUERY);
    css::uno::Any aValue = entryProps->getPropertyValue (OUString::createFromAscii ("Text"));
    OUString url;
    aValue >>= url;

    printf ("Accessing WebDAV server, trying location %s ...\n",
            OUStringToOString (url, RTL_TEXTENCODING_UTF8).getStr ());

    /* Create a reference to the SimpleFileAccess service */
    Reference< css::ucb::XSimpleFileAccess > fileAccess =
        Reference< css::ucb::XSimpleFileAccess > (
                mxMCF->createInstanceWithContext (OUString::createFromAscii ("com.sun.star.ucb.SimpleFileAccess"), mxContext), UNO_QUERY);

    if (!fileAccess.is ())
    {
        puts ("Could not create SimpleFileAccess object");
        return;
    }

    /* Set up an interaction handler, which will handle e.g. requesting
     * credentials from the user.
     */
    Reference< css::task::XInteractionHandler > interactionHandler =
        Reference< css::task::XInteractionHandler > (
                mxMCF->createInstanceWithContext (OUString::createFromAscii ("com.sun.star.task.InteractionHandler"), mxContext), UNO_QUERY);
    fileAccess->setInteractionHandler (interactionHandler);
    const Reference< XItemList > items(fileListModel, UNO_QUERY_THROW );
    items->removeAllItems();

    /* Now try to access the folder */
    try
    {
        if (! (fileAccess->exists (url) && fileAccess->isFolder (url)))
        {
            items->insertItemText(0, OUString::createFromAscii("(Failed to list documents)"));
            return;
        }
        Sequence< rtl::OUString > entries = fileAccess->getFolderContents (url, false);
        const OUString *stringArray = entries.getConstArray ();
        sal_Int32 n = entries.getLength ();

        OUString icon = OUString::createFromAscii ("file:///usr/share/icons/gnome/24x24/mimetypes/");
        for (sal_Int32 i = 0; i < n; i++)
        {
            /* Skip file formats that are not from LibreOffice */
            OUString extension (stringArray[i].copy (
                stringArray[i].lastIndexOf (OUString::createFromAscii (".")) + 1));
            if (! (extension.equalsAscii ("odf")
                || extension.equalsAscii ("odg")
                || extension.equalsAscii ("odm")
                || extension.equalsAscii ("odp")
                || extension.equalsAscii ("ods")
                || extension.equalsAscii ("odt")))
                continue;

            OUString fileName (stringArray[i].copy (
                stringArray[i].lastIndexOf (OUString::createFromAscii ("/")) + 1));
            items->insertItem(0, fileName,
                icon + OUString::createFromAscii ("x-office-document.png"));
            items->setItemData(0, makeAny (stringArray[i]));
        }
    }
    catch ( ... ) /* FIXME: Need proper exception handling here */
    {
        items->insertItemText(0, OUString::createFromAscii("(Failed to list documents)"));
    }
}
