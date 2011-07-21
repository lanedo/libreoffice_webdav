/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*************************************************************************
 *
 *  Copyright (C) 2011
 *  Michael Natterer <michael.natterer@lanedo.com>
 *  Kristian Rietveld <kris@lanedo.com>
 *  Christian Dywan <christian@lanedo.com>
 *  Lionel Dricot <lionel.dricot@lanedo.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General
 *  Public License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA 
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 *  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 *  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *************************************************************************/

#ifndef __WEBDAVDIALOG_HXX__
#define __WEBDAVDIALOG_HXX__

#include "settings.hxx"
#include <com/sun/star/awt/XToolkit.hpp>
#include <com/sun/star/awt/XItemListener.hpp>
#include <com/sun/star/frame/XFrame.hpp>
#include <com/sun/star/lang/XMultiComponentFactory.hpp>

namespace css = com::sun::star;

namespace WebDAVUI {

class FileDialog
{
private:
    css::uno::Reference< css::uno::XComponentContext> mxContext;
    css::uno::Reference< css::lang::XMultiComponentFactory > mxMCF;
    css::uno::Reference< css::frame::XFrame > mxFrame;
    css::uno::Reference< css::awt::XToolkit > mxToolkit;
    WebDAVUI::Settings* mSettings;
    css::uno::Reference< css::uno::XInterface > dialog;
    css::uno::Reference< css::uno::XInterface > locationEntryModel;
    css::uno::Reference< css::uno::XInterface > fileListModel;
    css::uno::Reference< css::uno::XInterface > fileEntryModel;

    sal_Bool isSave;

    void createDialog (void);
    bool showMessageBox (OUString errorMessage, bool confirm);

public:
    FileDialog( const css::uno::Reference< css::uno::XComponentContext > &rxContext,
                const css::uno::Reference< css::frame::XFrame >          &rxFrame,
                WebDAVUI::Settings*                                       mSettings,
                const sal_Bool                                            isSave);

    sal_Bool isSaveDialog (void);
    void show (void);
    void closeDialog (void);
    void openOrSaveSelectedDocument (void);
    void listFiles (void);
};

}

#endif /* __WEBDAVDIALOG_HXX__ */
