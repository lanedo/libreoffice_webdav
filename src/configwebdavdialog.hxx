#ifndef __CONFIGWEBDAVDIALOG_HXX__
#define __CONFIGWEBDAVDIALOG_HXX__

#include <com/sun/star/awt/XToolkit.hpp>
#include <com/sun/star/awt/XItemListener.hpp>
#include <com/sun/star/frame/XFrame.hpp>
#include <com/sun/star/lang/XMultiComponentFactory.hpp>

namespace css = com::sun::star;

class ConfigWebDAVDialog
{
private:
    css::uno::Reference< css::uno::XComponentContext> mxContext;
    css::uno::Reference< css::lang::XMultiComponentFactory > mxMCF;
    css::uno::Reference< css::frame::XFrame > mxFrame;
    css::uno::Reference< css::awt::XToolkit > mxToolkit;

    css::uno::Reference< css::uno::XInterface > dialog;

    void createDialog (void);

public:
    ConfigWebDAVDialog( const css::uno::Reference< css::uno::XComponentContext > &rxContext,
                  const css::uno::Reference< css::frame::XFrame >          &rxFrame);

    void show (void);
    void closeDialog (void);
    void showMessageBox (void);
};

#endif /* __CONFIGWEBDAVDIALOG_HXX__ */
