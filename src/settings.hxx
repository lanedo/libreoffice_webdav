#ifndef __WEBDAVUI_SETTINGS_HXX__
#define __WEBDAVUI_SETTINGS_HXX__

#include <com/sun/star/lang/XMultiComponentFactory.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/container/XNameContainer.hpp>

namespace css = com::sun::star;
using namespace css::uno;
using rtl::OUString;

namespace WebDAVUI {

class Settings
{
private:
    Reference< XComponentContext > mxContext;
    Reference< css::lang::XMultiComponentFactory > mxMCF;
    Reference< XInterface > mxIface;

    bool loadSettings (Reference< css::lang::XMultiServiceFactory > const & factory);
    bool getStringValueByReference (Reference< css::container::XNameAccess >& xAccess,
                                    const OUString& aKeyName, OUString& aValue);

public:
    Settings (const Reference< XComponentContext > &rxContext);

    OUString getStringValue (const OUString& aKeyName);
    OUString getRemoveServerName ();
};

}

#endif /* __WEBDAVUI_SETTINGS_HXX__ */
