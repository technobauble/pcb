/*!
 * \file src/actions/EditorActions.h
 *
 * \brief Editor utility actions (Atomic, MarkCrosshair).
 *
 * This module implements editor-level utility actions that don't directly
 * manipulate PCB objects but provide editor features.
 */

#ifndef PCB_ACTIONS_EDITORACTIONS_H
#define PCB_ACTIONS_EDITORACTIONS_H

#include "Action.h"

namespace pcb {
namespace actions {

//=============================================================================
// AtomicAction
//=============================================================================

class AtomicAction : public Action {
public:
    AtomicAction();
    int execute(int argc, char** argv, Coord x, Coord y) override;
};

//=============================================================================
// MarkCrosshairAction
//=============================================================================

class MarkCrosshairAction : public Action {
public:
    MarkCrosshairAction();
    int execute(int argc, char** argv, Coord x, Coord y) override;
};

} // namespace actions
} // namespace pcb

#endif // PCB_ACTIONS_EDITORACTIONS_H
