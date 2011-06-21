#include "webdavdialog.hxx"
#include <osl/diagnose.h>
#include <rtl/ustring.hxx>
#include <cppuhelper/implbase1.hxx>
#include <com/sun/star/awt/WindowAttribute.hpp>
#include <com/sun/star/awt/XButton.hpp>
#include <com/sun/star/awt/XControl.hpp>
#include <com/sun/star/awt/XControlContainer.hpp>
#include <com/sun/star/awt/XControlModel.hpp>
#include <com/sun/star/awt/XDialog.hpp>
#include <com/sun/star/awt/XWindowPeer.hpp>
#include <com/sun/star/awt/XMessageBox.hpp>
#include <com/sun/star/awt/XItemList.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/frame/XController.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/ucb/XSimpleFileAccess.hpp>
#include <com/sun/star/frame/XComponentLoader.hpp>
#include <com/sun/star/frame/FrameSearchFlag.hpp>

#include <cstdio> // TEMPORARY: for puts

using rtl::OUString;
using namespace css::awt;
using namespace css::beans;
using namespace css::container;
using namespace css::frame;
using namespace css::lang;
using namespace css::uno;
using css::lang::XMultiServiceFactory;

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
        puts ("dispose");
    }

    // XActionListener
    virtual void SAL_CALL actionPerformed (const css::awt::ActionEvent &rEvent) throw (css::uno::RuntimeException)
    {
        puts ("action performed");

        /* Obtain the name of the control the event originated from */
        Reference< XControl > control (rEvent.Source, UNO_QUERY);
        Reference< XControlModel > controlModel = control->getModel ();
        Reference< XPropertySet > controlProps (controlModel, UNO_QUERY);
        css::uno::Any aValue = controlProps->getPropertyValue (OUString::createFromAscii ("Name"));
        OUString controlName;
        aValue >>= controlName;

        if (controlName.equalsAscii ("Button1"))
        {
            /* FIXME: Is this okay with regard to threading, etc. ? */
            owner->openSelectedDocument ();
        }
        else if (controlName.equalsAscii ("Button2"))
        {
            /* FIXME: Is this okay with regard to threading, etc. ? */
            owner->dumpDAVListing ();
        }
    }
};


/* Dialog construction */

WebDAVDialog::WebDAVDialog( const Reference< css::lang::XMultiServiceFactory > &rxMSF,
                            const Reference< css::frame::XFrame > &rxFrame ) : mxMSF ( rxMSF ), mxFrame ( rxFrame )
{
    puts ("dialog ctor");

    // Create the toolkit to have access to it later
    mxToolkit = Reference< XToolkit >( mxMSF->createInstance(
                                        OUString( RTL_CONSTASCII_USTRINGPARAM(
                                            "com.sun.star.awt.Toolkit" ))), UNO_QUERY );

    createDialog ();
}

void WebDAVDialog::createDialog (void)
{
    /* The below code is a cleaned up and better commented version of
     * http://wiki.services.openoffice.org/wiki/Dialog_box_with_an_List_Box
     *
     *
     * In here, we hard code a dialog at run time.  Apparently, it
     * is also possible to design a dialog in the BASIC IDE, save
     * this to a .xdl (XDialog) file and load this at run time.
     * This is kind of like how glade works.  Might be a better
     * option.
     *
     * As far as I understand, the xdl file contains all models
     * like we define below in code.  We can attach "signal handlers"
     * at runtime by getting the created control through the
     * control container.
     */

    /* Create dialog model */
    Reference< XInterface > dialogModel =
        mxMSF->createInstance(OUString::createFromAscii("com.sun.star.awt.UnoControlDialogModel"));

    if (!dialogModel.is ())
    {
        printf ("Could not create XDialogModel.\n");
        return;
    }

    /* Configure the dialog by setting properties on the model */
    Reference< XPropertySet > dialogProps (dialogModel, UNO_QUERY);
    dialogProps->setPropertyValue(OUString::createFromAscii("PositionX"), makeAny ((sal_Int32) 100));
    dialogProps->setPropertyValue(OUString::createFromAscii("PositionY"), makeAny ((sal_Int32) 100));
    dialogProps->setPropertyValue(OUString::createFromAscii("Width"), makeAny ((sal_Int32) 150));
    dialogProps->setPropertyValue(OUString::createFromAscii("Height"), makeAny ((sal_Int32) 250));
    dialogProps->setPropertyValue(OUString::createFromAscii("Title"),
            makeAny (OUString::createFromAscii("Runtime Dialog Demo")));

    /* Create the actual control for the dialog and tie it to the
     * dialog model
     */
    dialog = mxMSF->createInstance(OUString::createFromAscii("com.sun.star.awt.UnoControlDialog"));
    Reference< XControl > control(dialog, UNO_QUERY);
    Reference< XControlModel > controlModel(dialogModel, UNO_QUERY);
    control->setModel(controlModel);

    /* Put the dialog in a window */
    Reference< XWindow > window (control, UNO_QUERY);
    window->setVisible(true);
    control->createPeer(mxToolkit,NULL);

    /* To create "child widgets" of the dialog, we need to use
     * a MultiServiceFactory tied to the dialog model.
     */
    Reference< XMultiServiceFactory > dialogMSF (dialogModel,
                                                 UNO_QUERY);

    /* Create a container for the controls within the dialog */
    Reference< XNameContainer > container (dialogModel, UNO_QUERY);

    /* Create an edit model for a text field */
    locationEntryModel =
        dialogMSF->createInstance(OUString::createFromAscii("com.sun.star.awt.UnoControlEditModel"));

    Reference< XPropertySet > entryProps (locationEntryModel, UNO_QUERY);

    entryProps->setPropertyValue(OUString::createFromAscii("PositionX"), makeAny ((sal_Int32) 10));
    entryProps->setPropertyValue(OUString::createFromAscii("PositionY"), makeAny ((sal_Int32) 40));
    entryProps->setPropertyValue(OUString::createFromAscii("Width"), makeAny ((sal_Int32) 130));
    entryProps->setPropertyValue(OUString::createFromAscii("Height"), makeAny ((sal_Int32) 16));
    entryProps->setPropertyValue(OUString::createFromAscii("Name"),
                                 makeAny (OUString::createFromAscii("LocationEntry")));
    entryProps->setPropertyValue(OUString::createFromAscii("Text"),
                                 makeAny (OUString::createFromAscii("http://localhost/dav/")));
    entryProps->setPropertyValue(OUString::createFromAscii("TabIndex"),makeAny((short)0));

    /* Add entry to container */
    container->insertByName (OUString::createFromAscii ("LocationEntry"),
                             makeAny (locationEntryModel));

    /* Create a button model and set properties */
    Reference< XInterface > buttonModel =
        dialogMSF->createInstance(OUString::createFromAscii("com.sun.star.awt.UnoControlButtonModel"));

    Reference< XPropertySet > buttonProps (buttonModel, UNO_QUERY);

    buttonProps->setPropertyValue(OUString::createFromAscii("PositionX"), makeAny ((sal_Int32) 10));
    buttonProps->setPropertyValue(OUString::createFromAscii("PositionY"), makeAny ((sal_Int32) 70));
    buttonProps->setPropertyValue(OUString::createFromAscii("Width"), makeAny ((sal_Int32) 50));
    buttonProps->setPropertyValue(OUString::createFromAscii("Height"), makeAny ((sal_Int32) 14));
    buttonProps->setPropertyValue(OUString::createFromAscii("Name"),
                                  makeAny (OUString::createFromAscii("Button1")));
    buttonProps->setPropertyValue(OUString::createFromAscii("TabIndex"),makeAny((short)1));

    buttonProps->setPropertyValue(OUString::createFromAscii("Label"),
                                  makeAny (OUString::createFromAscii("Open Document")));

    /* Add button to container */
    container->insertByName (OUString::createFromAscii("Button1"),
                             makeAny (buttonModel));

    /* Get the button control through the control container
     * (note that above we only created its model).
     */
    Reference< XControlContainer > controlContainer (dialog, UNO_QUERY);
    Reference< XInterface > buttonObject =
        controlContainer->getControl (OUString::createFromAscii ("Button1"));
    Reference< XButton > buttonControl (buttonObject, UNO_QUERY);

    /* and connect it to our action listener */
    Reference< XActionListener > actionListener =
        static_cast< XActionListener *> (new WebDAVDialogActionListener (this));
    buttonControl->addActionListener (actionListener);


    /* Second button */
    Reference< XInterface > buttonModel2 =
        dialogMSF->createInstance(OUString::createFromAscii("com.sun.star.awt.UnoControlButtonModel"));

    Reference< XPropertySet > buttonProps2 (buttonModel2, UNO_QUERY);

    buttonProps2->setPropertyValue(OUString::createFromAscii("PositionX"), makeAny ((sal_Int32) 70));
    buttonProps2->setPropertyValue(OUString::createFromAscii("PositionY"), makeAny ((sal_Int32) 70));
    buttonProps2->setPropertyValue(OUString::createFromAscii("Width"), makeAny ((sal_Int32) 70));
    buttonProps2->setPropertyValue(OUString::createFromAscii("Height"), makeAny ((sal_Int32) 14));
    buttonProps2->setPropertyValue(OUString::createFromAscii("Name"),
                                  makeAny (OUString::createFromAscii("Button2")));
    buttonProps2->setPropertyValue(OUString::createFromAscii("TabIndex"), makeAny((short)2));

    buttonProps2->setPropertyValue(OUString::createFromAscii("Label"),
                                  makeAny (OUString::createFromAscii("List contents of URL")));

    /* Add button to container */
    container->insertByName (OUString::createFromAscii("Button2"),
                             makeAny (buttonModel2));

    /* Get the button control through the control container
     * (note that above we only created its model).
     */
    Reference< XInterface > buttonObject2 =
        controlContainer->getControl (OUString::createFromAscii ("Button2"));
    Reference< XButton > buttonControl2 (buttonObject2, UNO_QUERY);

    buttonControl2->addActionListener (actionListener);

    /* Create an edit model for a text field outputting WebDAV contents */
    outputEntryModel =
        dialogMSF->createInstance(OUString::createFromAscii("com.sun.star.awt.UnoControlListBoxModel"));

    Reference< XPropertySet > entryProps2 (outputEntryModel, UNO_QUERY);
    Sequence < ::rtl::OUString > entries (1);
    entries[0] = ::rtl::OUString::createFromAscii ("(content listing will appear here)");

    entryProps2->setPropertyValue(OUString::createFromAscii("PositionX"), makeAny ((sal_Int32) 10));
    entryProps2->setPropertyValue(OUString::createFromAscii("PositionY"), makeAny ((sal_Int32) 90));
    entryProps2->setPropertyValue(OUString::createFromAscii("Width"), makeAny ((sal_Int32) 130));
    entryProps2->setPropertyValue(OUString::createFromAscii("Height"), makeAny ((sal_Int32) 150));
    entryProps2->setPropertyValue(OUString::createFromAscii("Name"),
                                 makeAny (OUString::createFromAscii("OutputEntry")));
    entryProps2->setPropertyValue(OUString::createFromAscii("StringItemList"), makeAny (entries));

    /* Add entry to container */
    container->insertByName (OUString::createFromAscii ("OutputEntry"),
                             makeAny (outputEntryModel));

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

void WebDAVDialog::openSelectedDocument (void)
{
    if ( ! (mxFrame.is() && mxToolkit.is()) )
        return;

    Reference< XPropertySet > entryProps (outputEntryModel, UNO_QUERY);
    css::uno::Any aValue = entryProps->getPropertyValue (OUString::createFromAscii ("SelectedItems"));
    Sequence< short > selectedItems;
    aValue >>= selectedItems;
    sal_Int32 n = selectedItems.getLength ();
    for (sal_Int32 i = 0; i < n; i++)
    {
        const Reference< XItemList > items( outputEntryModel, UNO_QUERY_THROW );
        css::uno::Any aURL = items->getItemData(selectedItems[0]);
        OUString sURL;
        aURL >>= sURL;
        printf ("Opening document: %s\n",
                OUStringToOString (sURL, RTL_TEXTENCODING_UTF8).getStr ());
        /* Save the frame name, invent a unique name, and restore the name later */
        OUString sOldName = mxFrame->getName();
        OUString sTarget = OUString::createFromAscii("odk_officedev_desk");
        mxFrame->setName(sTarget);
        Reference< css::frame::XComponentLoader > xLoader(mxFrame, UNO_QUERY);
        Sequence< css::beans::PropertyValue > lProperties (1);
        Reference< css::lang::XComponent > xDocument (xLoader->loadComponentFromURL(
            sURL, sTarget, css::frame::FrameSearchFlag::CHILDREN, lProperties));
        mxFrame->setName(sOldName);
    }
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
                mxMSF->createInstance (OUString::createFromAscii ("com.sun.star.ucb.SimpleFileAccess")), UNO_QUERY);

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
                mxMSF->createInstance (OUString::createFromAscii ("com.sun.star.task.InteractionHandler")), UNO_QUERY);
    fileAccess->setInteractionHandler (interactionHandler);
    const Reference< XItemList > items( outputEntryModel, UNO_QUERY_THROW );
    items->removeAllItems();

    /* Now try to access the folder */
    try
    {
        Sequence< rtl::OUString > entries = fileAccess->getFolderContents (url, false);
        const OUString *stringArray = entries.getConstArray ();
        sal_Int32 n = entries.getLength ();
        OUString icon = OUString::createFromAscii ("file:///usr/share/icons/gnome/24x24/mimetypes/");
        for (sal_Int32 i = 0; i < n; i++)
        {
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
