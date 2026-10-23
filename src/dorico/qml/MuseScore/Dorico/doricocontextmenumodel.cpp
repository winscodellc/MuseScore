/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include "doricocontextmenumodel.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMenu>
#include <QMimeData>
#include <QPointer>
#include <QQuickItem>

#include "engraving/dom/engravingitem.h"
#include "engraving/dom/mscore.h"
#include "notation/inotation.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationselection.h"
#include "translation.h"

using namespace mu::dorico;
using namespace mu::engraving;
using namespace muse::uicomponents;

namespace {
// Dorico's look (spec 98, "Style table"): dark, 6 px corners, 13 px, rows of about 18 px, no icons, blue highlight.
const char* const CONTEXT_MENU_STYLE = R"(
QMenu { background-color: rgba(46, 46, 46, 245); border: 1px solid #5c5c5c; border-radius: 6px; padding: 4px 0;
        font-size: 13px; color: #f2f2f2; }
QMenu::item { padding: 2px 24px 2px 22px; min-height: 14px; background: transparent; }
QMenu::item:selected { background-color: #3b78e7; border-radius: 4px; margin: 0 4px; }
QMenu::item:disabled { color: #8c8c8c; }
QMenu::separator { height: 1px; background: #5c5c5c; margin: 4px 0; }
QMenu::indicator { width: 14px; }
)";

bool isTextItem(ElementType type)
{
    return type == ElementType::TEMPO_TEXT || type == ElementType::STAFF_TEXT || type == ElementType::SYSTEM_TEXT
           || type == ElementType::EXPRESSION || type == ElementType::TEXT;
}

void fillMenu(QMenu* menu, const MenuItemList& items, const std::function<void(const QString&)>& chosen)
{
    for (const MenuItem* item : items) {
        if (item->id().isEmpty() && item->subitems().isEmpty()) {
            menu->addSeparator();
            continue;
        }
        QString title = item->translatedTitle();
        title.replace("&", "&&"); // no accelerators: "&" is a literal
        if (!item->subitems().isEmpty()) {
            QMenu* submenu = menu->addMenu(title);
            fillMenu(submenu, item->subitems(), chosen);
            continue;
        }
        const QString shortcut = item->shortcutsTitle().section(", ", 0, 0);
        QAction* action = menu->addAction(shortcut.isEmpty() ? title : title + "\t" + shortcut);
        action->setEnabled(item->enabled());
        if (item->id().startsWith("dorico-ctx-")) { // a stub; a real command is disabled only by its state
            action->setToolTip(muse::qtrc("dorico", "Not available yet"));
        }
        if (item->checked()) {
            action->setCheckable(true);
            action->setChecked(true);
        }
        const QString id = item->id();
        QObject::connect(action, &QAction::triggered, menu, [chosen, id]() { chosen(id); });
    }
}
}

DoricoContextMenuModel::DoricoContextMenuModel(QObject* parent)
    : AbstractMenuModel(parent)
{
}

ContextMenuInput DoricoContextMenuModel::inputFromSelection() const
{
    ContextMenuInput in;
    in.mode = modes()->mode();
    // a copied range is a staff list, a copied single item or selection of items a symbol / symbol list
    const QMimeData* clip = QGuiApplication::clipboard() ? QGuiApplication::clipboard()->mimeData() : nullptr;
    in.clipboardHasMusic = clip && (clip->hasFormat(QString::fromLatin1(mimeStaffListFormat))
                                    || clip->hasFormat(QString::fromLatin1(mimeSymbolFormat))
                                    || clip->hasFormat(QString::fromLatin1(mimeSymbolListFormat)));

    const notation::INotationPtr notation = globalContext()->currentNotation();
    const notation::INotationSelectionPtr selection = notation ? notation->interaction()->selection() : nullptr;
    if (!selection || selection->isNone()) {
        return in;
    }
    in.hasSelection = true;
    if (selection->isRange()) {
        // a range holds notes, and Dorico lists the dynamics and text rows too
        in.notes = in.dynamics = in.text = true;
        return in;
    }
    for (const EngravingItem* e : selection->elements()) {
        const ElementType type = e->type();
        in.notes |= type == ElementType::NOTE || type == ElementType::CHORD || type == ElementType::REST;
        in.dynamics |= type == ElementType::DYNAMIC;
        in.slurs |= type == ElementType::SLUR_SEGMENT;
        in.text |= isTextItem(type);
    }
    return in;
}

MenuItemList DoricoContextMenuModel::build(const std::vector<MenuSpec>& specs)
{
    MenuItemList items;
    for (const MenuSpec& spec : specs) {
        const muse::TranslatableString title("dorico", spec.title.toUtf8().constData());
        if (spec.isSeparator()) {
            items << makeSeparator();
        } else if (spec.isSubmenu()) {
            items << makeMenu(title, build(spec.children), spec.id);
        } else if (spec.id.startsWith("command://")
                   && commandsRegister()->commandInfo(muse::rcommand::Command(spec.id.toStdString())).isValid()) {
            items << makeMenuItem(muse::rcommand::Command(spec.id.toStdString()), title);
        } else {
            // dorico-setup-*: a panel action, enabled. dorico-ctx-*: a stub with no behaviour yet, disabled.
            auto* item = new MenuItem(this);
            item->setId(spec.id);
            item->setTitle(title);
            item->setEnabled(spec.id.startsWith("dorico-setup-"));
            items << item;
        }
    }
    return items;
}

void DoricoContextMenuModel::replaceItems(const std::vector<MenuSpec>& specs)
{
    // Every popup builds a new tree owned by this model: free the last one. An open QMenu only holds copied ids.
    const QList<MenuItem*> old = findChildren<MenuItem*>(Qt::FindDirectChildrenOnly);
    setItems(build(specs));
    qDeleteAll(old);
}

void DoricoContextMenuModel::popup(QQuickItem* view, const QPointF& viewPos)
{
    AbstractMenuModel::load();
    replaceItems(scoreContextMenu(inputFromSelection()));
    if (view && !items().isEmpty()) {
        show(view, viewPos, QString());
    }
}

void DoricoContextMenuModel::popupSetup(const QString& kind, QQuickItem* anchor, const QPointF& pos, const QStringList& disabledIds)
{
    AbstractMenuModel::load();
    replaceItems(kind == "player" ? playerContextMenu() : kind == "flow" ? flowContextMenu() : layoutContextMenu());
    for (MenuItem* item : items()) {
        if (disabledIds.contains(item->id())) {
            item->setEnabled(false);
        }
    }
    if (anchor) {
        show(anchor, pos, kind);
    }
}

void DoricoContextMenuModel::show(QQuickItem* anchor, const QPointF& pos, const QString& kind)
{
    auto* menu = new QMenu();
    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->setAttribute(Qt::WA_TranslucentBackground);
    menu->setWindowFlags(menu->windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    menu->setStyleSheet(CONTEXT_MENU_STYLE);
    menu->setToolTipsVisible(true);

    QPointer<DoricoContextMenuModel> self(this);
    fillMenu(menu, items(), [self, kind](const QString& id) {
        if (!self) {
            return;
        }
        if (id.startsWith("dorico-setup-")) {
            emit self->setupItemChosen(kind, id);
        } else {
            self->handleMenuItem(id);
        }
    });

    // The paint view hides its own popups while a menu is open, and needs to know when this one closes.
    QPointer<QQuickItem> view(anchor);
    const bool isScore = kind.isEmpty();
    const auto notifyOpen = [view, isScore](bool open) {
        if (view && isScore) {
            QMetaObject::invokeMethod(view, "onContextMenuIsOpenChanged", Q_ARG(bool, open));
        }
    };
    notifyOpen(true);
    QObject::connect(menu, &QMenu::aboutToHide, menu, [notifyOpen]() { notifyOpen(false); });
    menu->popup(anchor->mapToGlobal(pos).toPoint());
}
