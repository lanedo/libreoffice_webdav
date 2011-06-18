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
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/frame/XController.hpp>
#include <com/sun/star/lang/XComponent.hpp>

#include <cstdio> // TEMPORARY: for puts

using rtl::OUString;
using namespace com::sun::star::awt;
using namespace com::sun::star::beans;
using namespace com::sun::star::container;
using namespace com::sun::star::frame;
using namespace com::sun::star::lang;
using namespace com::sun::star::uno;
using com::sun::star::lang::XMultiServiceFactory;

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

        /* FIXME: Is this okay with regard to threading, etc. ? */
        owner->showMessageBox ();
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
    dialogProps->setPropertyValue(OUString::createFromAscii("PositionX"), makeAny (100));
    dialogProps->setPropertyValue(OUString::createFromAscii("PositionY"), makeAny (100));
    dialogProps->setPropertyValue(OUString::createFromAscii("Width"), makeAny (150));
    dialogProps->setPropertyValue(OUString::createFromAscii("Height"), makeAny (100));
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

    /* Create a button model and set properties */
    Reference< XInterface > buttonModel =
        dialogMSF->createInstance(OUString::createFromAscii("com.sun.star.awt.UnoControlButtonModel"));

    Reference< XPropertySet > buttonProps (buttonModel, UNO_QUERY);

    buttonProps->setPropertyValue(OUString::createFromAscii("PositionX"), makeAny (20));
    buttonProps->setPropertyValue(OUString::createFromAscii("PositionY"), makeAny (70));
    buttonProps->setPropertyValue(OUString::createFromAscii("Width"), makeAny (50));
    buttonProps->setPropertyValue(OUString::createFromAscii("Height"), makeAny (14));
    buttonProps->setPropertyValue(OUString::createFromAscii("Name"),
                                  makeAny (OUString::createFromAscii("Button1")));
    buttonProps->setPropertyValue(OUString::createFromAscii("TabIndex"),makeAny((short)0));

    buttonProps->setPropertyValue(OUString::createFromAscii("Label"),
                                  makeAny (OUString::createFromAscii("Show Dialog")));

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
    Reference< XActionListener > actionListener =
        static_cast< XActionListener *> (new WebDAVDialogActionListener (this));
    buttonControl->addActionListener (actionListener);


    /* Second button */
    Reference< XInterface > buttonModel2 =
        dialogMSF->createInstance(OUString::createFromAscii("com.sun.star.awt.UnoControlButtonModel"));

    Reference< XPropertySet > buttonProps2 (buttonModel2, UNO_QUERY);

    buttonProps2->setPropertyValue(OUString::createFromAscii("PositionX"), makeAny (80));
    buttonProps2->setPropertyValue(OUString::createFromAscii("PositionY"), makeAny (70));
    buttonProps2->setPropertyValue(OUString::createFromAscii("Width"), makeAny (50));
    buttonProps2->setPropertyValue(OUString::createFromAscii("Height"), makeAny (14));
    buttonProps2->setPropertyValue(OUString::createFromAscii("Name"),
                                  makeAny (OUString::createFromAscii("Button2")));
    buttonProps2->setPropertyValue(OUString::createFromAscii("TabIndex"), makeAny((short)1));

    buttonProps2->setPropertyValue(OUString::createFromAscii("Label"),
                                  makeAny (OUString::createFromAscii("Dummy Button")));

    /* Add button to container */
    container->insertByName (OUString::createFromAscii("Button2"),
                             makeAny (buttonModel2));
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
