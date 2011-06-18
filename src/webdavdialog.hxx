#ifndef __WEBDAVDIALOG_HXX__
#define __WEBDAVDIALOG_HXX__

#include <com/sun/star/awt/XToolkit.hpp>
#include <com/sun/star/frame/XFrame.hpp>
#include <com/sun/star/lang/XInitialization.hpp>
#include <cppuhelper/implbase1.hxx>

namespace css = com::sun::star;

class WebDAVDialog : public cppu::WeakImplHelper1
<
    ::css::lang::XInitialization
>
{
private:
    css::uno::Reference< css::lang::XMultiServiceFactory > mxMSF;
    css::uno::Reference< css::frame::XFrame > mxFrame;
    css::uno::Reference< css::awt::XToolkit > mxToolkit;

public:
    WebDAVDialog( const css::uno::Reference< css::lang::XMultiServiceFactory > &rxMSF,
                  const css::uno::Reference< css::frame::XFrame > &rxFrame );

    void show (void);

    // XInitialization
    virtual void SAL_CALL initialize( const ::com::sun::star::uno::Sequence< ::com::sun::star::uno::Any >& aArguments )
        throw (::com::sun::star::uno::Exception, ::com::sun::star::uno::RuntimeException);
};

#endif /* __WEBDAVDIALOG_HXX__ */
