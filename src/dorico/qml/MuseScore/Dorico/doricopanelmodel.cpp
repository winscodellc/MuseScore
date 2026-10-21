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

#include "doricopanelmodel.h"

#include <map>

#include "log.h"

#include "dockwindow/idockwindow.h"
#include "palette/internal/palettecreator.h"
#include "palette/internal/palettetree.h"

#include "../../../internal/notationspanels.h"

using namespace mu::dorico;
using namespace mu::palette;

static const QString NOTATIONS_PANEL_DOCK("doricoRightZone"); // the right zone dock (spec 95), on Notations in Write

DoricoPanelModel::DoricoPanelModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

void DoricoPanelModel::setPanelId(const QString& id)
{
    if (m_panelId == id) {
        return;
    }
    m_panelId = id;
    rebuild();
    emit panelChanged();
}

static PalettePtr makeSourcePalette(PaletteCreator& creator, SourcePalette source)
{
    // The master palettes (all cells), not the shorter default ones.
    switch (source) {
    case SourcePalette::Clefs: return creator.newClefsPalette();
    case SourcePalette::KeySignatures: return creator.newKeySigPalette();
    case SourcePalette::TimeSignatures: return creator.newTimePalette();
    case SourcePalette::Tempo: return creator.newTempoPalette();
    case SourcePalette::Accidentals: return creator.newAccidentalsPalette();
    case SourcePalette::Dynamics: return creator.newDynamicsPalette();
    case SourcePalette::Articulations: return creator.newArticulationsPalette();
    case SourcePalette::Text: return creator.newTextPalette();
    case SourcePalette::Keyboard: return creator.newKeyboardPalette();
    case SourcePalette::Repeats: return creator.newRepeatsPalette();
    case SourcePalette::BarLines: return creator.newBarLinePalette();
    case SourcePalette::Ornaments: return creator.newOrnamentsPalette();
    case SourcePalette::Breaths: return creator.newBreathPalette();
    case SourcePalette::Arpeggios: return creator.newArpeggioPalette();
    case SourcePalette::Tremolos: return creator.newTremoloPalette();
    case SourcePalette::Harp: return creator.newHarpPalette();
    case SourcePalette::Guitar: return creator.newGuitarPalette();
    case SourcePalette::Lines: return creator.newLinesPalette();
    case SourcePalette::Brackets: return creator.newBracketsPalette();
    }
    return nullptr;
}

void DoricoPanelModel::rebuild()
{
    const std::vector<NotationsSection> sections = sectionsOfPanel(m_panelId);

    PaletteCreator creator(iocContext());
    std::map<SourcePalette, PalettePtr> sources;
    std::map<SourcePalette, std::vector<PaletteCellInfo> > infos;
    for (const NotationsSection& section : sections) {
        if (sources.count(section.palette)) {
            continue;
        }
        PalettePtr palette = makeSourcePalette(creator, section.palette);
        std::vector<PaletteCellInfo>& info = infos[section.palette];
        for (const PaletteCellPtr& cell : palette->cells()) {
            info.push_back({ cell->element ? cell->element->type() : engraving::ElementType::INVALID, cell->name });
        }
        sources[section.palette] = palette;
    }

    QStringList unknown;
    const auto resolved = resolveSections(sections, [&](SourcePalette p) -> const std::vector<PaletteCellInfo>& {
        return infos[p];
    }, &unknown);
    if (!unknown.isEmpty()) {
        LOGW() << "Dorico panel " << m_panelId << ": no palette cell for " << unknown.join(", ");
    }

    m_tree = std::make_shared<PaletteTree>();
    m_titles.clear();
    for (size_t s = 0; s < sections.size(); ++s) {
        // A section with no MuseScore cell is left out rather than shown empty.
        if (resolved[s].empty()) {
            continue;
        }
        const PalettePtr& source = sources[sections[s].palette];
        auto palette = std::make_shared<Palette>(iocContext(), source->type());
        palette->setName(sections[s].title);
        palette->setGridSize(source->gridSize());
        palette->setMag(source->mag());
        palette->setDrawGrid(source->drawGrid());
        palette->setYOffset(source->yOffset());
        for (size_t i : resolved[s]) {
            palette->insertCell(palette->cellsCount(), source->cellAt(i));
        }
        m_tree->append(palette);
        m_titles << muse::qtrc("dorico", sections[s].title.toUtf8().constData());
    }

    delete m_controller;
    delete m_model;
    m_model = new PaletteTreeModel(m_tree, iocContext(), this);
    m_controller = new UserPaletteController(m_model, m_model, iocContext(), this);
    // The panel is a fixed view: no dragging cells around or deleting them.
    m_controller->setUserEditable(false);
}

QModelIndex DoricoPanelModel::sectionIndex(int section) const
{
    return m_model ? m_model->index(section, 0) : QModelIndex();
}

QSize DoricoPanelModel::cellSize(int section) const
{
    if (!m_tree || section < 0 || section >= static_cast<int>(m_tree->palettes.size())) {
        return QSize();
    }
    return m_tree->palettes[section]->scaledGridSize();
}

bool DoricoPanelModel::drawGrid(int section) const
{
    if (!m_tree || section < 0 || section >= static_cast<int>(m_tree->palettes.size())) {
        return false;
    }
    return m_tree->palettes[section]->drawGrid();
}

void DoricoPanelModel::setPanelOpen(bool open)
{
    if (auto window = dockWindowProvider()->window()) {
        window->setDockOpen(NOTATIONS_PANEL_DOCK, open);
    }
}

bool DoricoPanelModel::isPanelOpen() const
{
    const auto window = dockWindowProvider()->window();
    return window && window->isDockOpen(NOTATIONS_PANEL_DOCK);
}
