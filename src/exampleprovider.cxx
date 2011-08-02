#include "exampleprovider.hxx"

#include <cppuhelper/queryinterface.hxx>
#include <cppuhelper/typeprovider.hxx>
#include <osl/mutex.hxx>

#ifdef DEBUG
    #include <cstdio>
#else
    #define printf(...)
#endif

using rtl::OUString;
using namespace css::ucb;
using namespace css::uno;
using namespace css::lang;

ExampleContentProvider::ExampleContentProvider (
    const Reference <XMultiServiceFactory> &rxMSF) : mxMSF (rxMSF)
{
}

/* XInterface */
void SAL_CALL ExampleContentProvider::acquire()
    throw()
{
    OWeakObject::acquire();
}

void SAL_CALL ExampleContentProvider::release()
    throw()
{
    OWeakObject::release();
}

Any SAL_CALL ExampleContentProvider::queryInterface (const Type &rType)
    throw (RuntimeException)
{
    Any ret = cppu::queryInterface (rType,
            static_cast<XTypeProvider*>(this),
            static_cast<XServiceInfo*>(this),
            static_cast<XContentProvider*>(this)
            );

    return ret.hasValue () ? ret : OWeakObject::queryInterface (rType);
}

/* XTypeProvider */
Sequence <sal_Int8> SAL_CALL ExampleContentProvider::getImplementationId ()
    throw (RuntimeException)
{
    static cppu::OImplementationId *pId = NULL;
    if (!pId)
    {
        osl::Guard<osl::Mutex> guard (osl::Mutex::getGlobalMutex ());
        if (!pId)
        {
            static cppu::OImplementationId id (sal_False);
            pId = &id;
        }
    }
    return (*pId).getImplementationId();
}

Sequence< ::Type > SAL_CALL ExampleContentProvider::getTypes ()
    throw (RuntimeException)
{
    static cppu::OTypeCollection *pCollection = NULL;
    if (!pCollection)
    {
        osl::Guard<osl::Mutex> guard (osl::Mutex::getGlobalMutex ());
        if (!pCollection)
        {
            static cppu::OTypeCollection collection (
                    getCppuType (static_cast<Reference<XTypeProvider>*>(0)),
                    getCppuType (static_cast<Reference<XServiceInfo>*>(0)),
                    getCppuType (static_cast<Reference<XContentProvider>*>(0))
                    );
            pCollection = &collection;
        }
    }

    return (*pCollection).getTypes();
}

/* XServiceInfo */
rtl::OUString SAL_CALL ExampleContentProvider::getImplementationName ()
    throw (RuntimeException)
{
    return rtl::OUString (RTL_CONSTASCII_USTRINGPARAM (SERVICE_NAME));
}

sal_Bool SAL_CALL ExampleContentProvider::supportsService(const OUString &serviceName)
    throw (RuntimeException)
{
    printf ("ExampleContentProvider::supportsService %s\n",
            OUStringToOString (serviceName, RTL_TEXTENCODING_UTF8).getStr ());
    return serviceName.equalsAsciiL (RTL_CONSTASCII_STRINGPARAM (SERVICE_NAME));
}

Sequence <OUString> SAL_CALL ExampleContentProvider::getSupportedServiceNames ()
    throw (RuntimeException)
{
    return getSupportedServiceNames_Static();
}

Sequence <OUString> SAL_CALL ExampleContentProvider::getSupportedServiceNames_Static ()
    throw (RuntimeException)
{
    printf ("ExampleContentProvider::getSupportedServiceNames_Static\n");
    Sequence <OUString> ret (1);
    OUString *array = ret.getArray ();

    array[0] = OUString (RTL_CONSTASCII_USTRINGPARAM (SERVICE_NAME));

    return ret;
}

/* XContentProvider */
Reference <XContent> SAL_CALL ExampleContentProvider::queryContent (
    const Reference <XContentIdentifier> &Identifier)
    throw (IllegalIdentifierException, RuntimeException)
{
    OUString url = Identifier->getContentIdentifier ();
    if (!url.match (OUString::createFromAscii ("example://"), 0))
        throw IllegalIdentifierException ();
    printf ("ExampleContentProvider::queryContent %s\n",
            OUStringToOString (url, RTL_TEXTENCODING_UTF8).getStr ());
    return NULL;
}

sal_Int32 SAL_CALL ExampleContentProvider::compareContentIds (const Reference <XContentIdentifier> &id1,
                                                              const Reference <XContentIdentifier> &id2)
    throw (RuntimeException)
{
    rtl::OUString url1 (id1->getContentIdentifier ());
    rtl::OUString url2 (id2->getContentIdentifier ());

    return url1.compareTo (url2);
}

Reference<XInterface> SAL_CALL ExampleContentProvider_create(const Reference<XMultiServiceFactory> &xMgr)
{
    printf ("ExampleContentProvider_create\n");
    return Reference<XInterface>(static_cast<XContentProvider*>(new ExampleContentProvider(xMgr)));
}
