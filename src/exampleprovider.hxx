#ifndef _EXAMPLE_UCP_PROVIDER_HXX
#define _EXAMPLE_UCP_PROVIDER_HXX

#include <sal/types.h>
#include <com/sun/star/ucb/XContent.hpp>
#include <com/sun/star/ucb/XContentProvider.hpp>
#include <com/sun/star/ucb/XContentIdentifier.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/lang/XTypeProvider.hpp>
#include <cppuhelper/weak.hxx>

namespace css = com::sun::star;
using namespace css::uno;

#define SERVICE_NAME "com.lanedo.ExampleContentProvider"

class ExampleContentProvider : public cppu::OWeakObject,
                               public css::lang::XTypeProvider,
                               public css::lang::XServiceInfo,
                               public css::ucb::XContentProvider
{
private:
    Reference<css::lang::XMultiServiceFactory> mxMSF;

public:
    ExampleContentProvider (const Reference<css::lang::XMultiServiceFactory> &rxMSF);

    /* XInterface */
    virtual Any SAL_CALL queryInterface (const css::uno::Type &rType)
            throw (RuntimeException);
    virtual void SAL_CALL acquire ()
            throw ();
    virtual void SAL_CALL release ()
            throw ();

    /* XTypeProvider */
    virtual Sequence<sal_Int8> SAL_CALL getImplementationId ()
            throw (RuntimeException);
    virtual Sequence<css::uno::Type> SAL_CALL getTypes ()
            throw (RuntimeException);

    /* XServiceInfo */
    virtual rtl::OUString SAL_CALL getImplementationName ()
            throw (RuntimeException);
    virtual sal_Bool SAL_CALL supportsService (const rtl::OUString &serviceName)
            throw (css::uno::RuntimeException);
    virtual Sequence< rtl::OUString > SAL_CALL getSupportedServiceNames ()
            throw (RuntimeException);
    static Sequence< rtl::OUString > SAL_CALL getSupportedServiceNames_Static ()
            throw (RuntimeException);

    /* XContentProvider */
    virtual css::uno::Reference < css::ucb::XContent > SAL_CALL queryContent (const css::uno::Reference<css::ucb::XContentIdentifier > & identifier)
            throw (css::ucb::IllegalIdentifierException, RuntimeException);
    virtual sal_Int32 SAL_CALL compareContentIds (const Reference <css::ucb::XContentIdentifier> & Id1,
                                                  const Reference <css::ucb::XContentIdentifier> & Id2)
            throw (RuntimeException);
};


Reference<XInterface> SAL_CALL ExampleContentProvider_create(
    const Reference<css::lang::XMultiServiceFactory> &xMgr);

#endif /* _EXAMPLE_UCP_PROVIDER_HXX_ */
