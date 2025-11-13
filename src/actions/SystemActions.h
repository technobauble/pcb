/*!
 * \file src/actions/SystemActions.h
 *
 * \brief System-level actions (Quit, Message, DumpLibrary).
 *
 * This module implements simple system-level actions that don't manipulate
 * PCB data directly.
 */

#ifndef PCB_ACTIONS_SYSTEMACTIONS_H
#define PCB_ACTIONS_SYSTEMACTIONS_H

#include "Action.h"

namespace pcb {
namespace actions {

//=============================================================================
// QuitAction
//=============================================================================

class QuitAction : public Action {
public:
    QuitAction();
    int execute(int argc, char** argv, Coord x, Coord y) override;
};

//=============================================================================
// MessageAction
//=============================================================================

class MessageAction : public Action {
public:
    MessageAction();
    int execute(int argc, char** argv, Coord x, Coord y) override;
};

//=============================================================================
// DumpLibraryAction
//=============================================================================

class DumpLibraryAction : public Action {
public:
    DumpLibraryAction();
    int execute(int argc, char** argv, Coord x, Coord y) override;
};

} // namespace actions
} // namespace pcb

#endif // PCB_ACTIONS_SYSTEMACTIONS_H
