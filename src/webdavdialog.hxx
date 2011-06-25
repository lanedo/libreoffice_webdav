#ifndef __WEBDAVDIALOG_HXX__
#define __WEBDAVDIALOG_HXX__

#include <com/sun/star/awt/XToolkit.hpp>
#include <com/sun/star/awt/XItemListener.hpp>
#include <com/sun/star/frame/XFrame.hpp>
#include <com/sun/star/lang/XMultiComponentFactory.hpp>

namespace css = com::sun::star;

class WebDAVDialog
{
private:
    css::uno::Reference< css::uno::XComponentContext> mxContext;
    css::uno::Reference< css::lang::XMultiComponentFactory > mxMCF;
    css::uno::Reference< css::frame::XFrame > mxFrame;
    css::uno::Reference< css::awt::XToolkit > mxToolkit;

    css::uno::Reference< css::uno::XInterface > dialog;
    css::uno::Reference< css::uno::XInterface > locationEntryModel;
    css::uno::Reference< css::uno::XInterface > fileListModel;

    sal_Bool isSave;

    void createDialog (void);

public:
    WebDAVDialog( const css::uno::Reference< css::uno::XComponentContext > &rxContext,
                  const css::uno::Reference< css::frame::XFrame >          &rxFrame,
                  const sal_Bool                                            isSave);

    sal_Bool isSaveDialog (void);
    void show (void);
    void showMessageBox (void);
    void openSelectedDocument (void);
    void saveSelectedDocument (void);
    void dumpDAVListing (void);
};

#endif /* __WEBDAVDIALOG_HXX__ */
