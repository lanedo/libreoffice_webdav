#include "configwebdavdialog.hxx"
#include <osl/diagnose.h>
#include <rtl/ustring.hxx>
#include <cppuhelper/implbase1.hxx>
#include <com/sun/star/awt/Key.hpp>
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

/* Action listener */
class WebDAVDialogActionListener : public ::cppu::WeakImplHelper1< css::awt::XActionListener >
{
private:
    ConfigWebDAVDialog * const owner;

public:
    WebDAVDialogActionListener (ConfigWebDAVDialog * const _owner)
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

        if (controlName.equalsAscii ("SaveButton"))
        {
            owner->closeDialog ();
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
    ConfigWebDAVDialog * const owner;

public:
    WebDAVDialogKeyListener (ConfigWebDAVDialog * const _owner)
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
    }

    virtual void SAL_CALL keyReleased (const css::awt::KeyEvent &rEvent) throw (css::uno::RuntimeException)
    {
        puts ("key released");
    }
};


/* Dialog construction */

ConfigWebDAVDialog::ConfigWebDAVDialog( const Reference< css::uno::XComponentContext > &rxContext,
                            const Reference< css::frame::XFrame >          &rxFrame) : mxContext ( rxContext ),
                                                                                      mxFrame ( rxFrame )
{
    puts ("dialog ctor");

    mxMCF = mxContext->getServiceManager ();

    // Create the toolkit to have access to it later
    mxToolkit = Reference< XToolkit >( mxMCF->createInstanceWithContext(
                                        OUString( RTL_CONSTASCII_USTRINGPARAM(
                                            "com.sun.star.awt.Toolkit" )), mxContext), UNO_QUERY );

    createDialog ();
}

void ConfigWebDAVDialog::createDialog (void)
{
    /* Construct path to XDL file in extension package */
    Reference< XPackageInformationProvider> infoProvider =
        PackageInformationProvider::get (mxContext);

    OUString dialogFile(RTL_CONSTASCII_USTRINGPARAM("/config.xdl"));
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
    realDialog->setTitle(OUString::createFromAscii("Configure the Cloud"));

    /* Put the dialog in a window */
    Reference< XControl > control(dialog, UNO_QUERY);
    Reference< XWindow > window(control, UNO_QUERY);
    window->setVisible(true);
    control->createPeer(mxToolkit,NULL);

    /* Get the open/save button */
    Reference< XControlContainer > controlContainer (dialog, UNO_QUERY);
    Reference< XControl > saveButton =
        controlContainer->getControl (OUString::createFromAscii ("SaveButton"));
    Reference< XControlModel > saveButtonModel =
        saveButton->getModel ();

    Reference< XPropertySet > openProps (saveButtonModel, UNO_QUERY);

    /* FIXME, these strings need to be translatable */
    openProps->setPropertyValue(OUString::createFromAscii("Label"),
                                  makeAny (OUString::createFromAscii("Save Config")));


    /* Create event listeners */
    Reference< XActionListener > actionListener =
        static_cast< XActionListener *> (new WebDAVDialogActionListener (this));

    Reference< XKeyListener > keyListener =
        static_cast< XKeyListener *> (new WebDAVDialogKeyListener (this));

    Reference< XButton > saveButtonControl (saveButton, UNO_QUERY);
    saveButtonControl->addActionListener (actionListener);


    /* Connect cancel button to action listener */
    Reference< XInterface > cancelObject =
        controlContainer->getControl (OUString::createFromAscii ("CancelButton"));
    Reference< XButton > cancelControl (cancelObject, UNO_QUERY);

    cancelControl->addActionListener (actionListener);
}


void ConfigWebDAVDialog::show (void)
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

void ConfigWebDAVDialog::closeDialog (void)
{
    Reference< XDialog > xDialog(dialog,UNO_QUERY);
    xDialog->endExecute();
}

void ConfigWebDAVDialog::showMessageBox (void)
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
