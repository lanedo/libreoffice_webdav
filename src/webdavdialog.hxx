#ifndef __WEBDAVDIALOG_HXX__
#define __WEBDAVDIALOG_HXX__

#include <com/sun/star/awt/XToolkit.hpp>
#include <com/sun/star/frame/XFrame.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>

namespace css = com::sun::star;

class WebDAVDialog
{
private:
    css::uno::Reference< css::lang::XMultiServiceFactory > mxMSF;
    css::uno::Reference< css::frame::XFrame > mxFrame;
    css::uno::Reference< css::awt::XToolkit > mxToolkit;

    css::uno::Reference< css::uno::XInterface > dialog;
    css::uno::Reference< css::uno::XInterface > locationEntryModel;
    css::uno::Reference< css::uno::XInterface > outputEntryModel;

    void createDialog (void);

public:
    WebDAVDialog( const css::uno::Reference< css::lang::XMultiServiceFactory > &rxMSF,
                  const css::uno::Reference< css::frame::XFrame > &rxFrame );

    void show (void);
    void showMessageBox (void);
    void dumpDAVListing (void);
};

#endif /* __WEBDAVDIALOG_HXX__ */
