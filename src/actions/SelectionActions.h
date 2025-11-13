/*!
 * \file src/actions/SelectionActions.h
 *
 * \brief Header for selection-related actions.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2025 PCB Contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef PCB_SELECTION_ACTIONS_H
#define PCB_SELECTION_ACTIONS_H

#include "Action.h"

namespace pcb {
namespace actions {

/*!
 * \brief Action to select objects on the PCB.
 *
 * Supports multiple selection modes:
 * - Object/ToggleObject: Select object under cursor
 * - Block: Select all objects in selection rectangle
 * - All: Select all visible objects
 * - Found: Select objects marked as 'found'
 * - Connection: Select physically connected objects
 * - BuriedVias: Select buried vias
 * - *ByName: Select objects matching a regex pattern
 */
class SelectAction : public Action {
public:
    SelectAction();
    int execute(int argc, char** argv, Coord x, Coord y) override;

private:
    void selectByName(int type, const char* pattern);
};

/*!
 * \brief Action to unselect objects on the PCB.
 *
 * Supports the same modes as SelectAction but clears selection
 * instead of setting it.
 */
class UnselectAction : public Action {
public:
    UnselectAction();
    int execute(int argc, char** argv, Coord x, Coord y) override;

private:
    void unselectByName(int type, const char* pattern);
};

/*!
 * \brief Action to remove all selected objects from the board.
 *
 * This is a simple action that removes everything currently selected.
 */
class RemoveSelectedAction : public Action {
public:
    RemoveSelectedAction();
    int execute(int argc, char** argv, Coord x, Coord y) override;
};

} // namespace actions
} // namespace pcb

#endif // PCB_SELECTION_ACTIONS_H
