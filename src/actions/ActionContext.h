/*!
 * \file src/actions/ActionContext.h
 *
 * \brief Shared state context for PCB actions
 *
 * This header defines the ActionContext structure that holds shared state
 * previously maintained as file-scoped static variables in action.c.
 *
 * By centralizing this state, we:
 * - Enable C++ actions to access shared state
 * - Improve testability (can mock the context)
 * - Document all shared state in one place
 * - Enable future refactoring (e.g., multi-document support)
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef PCB_ACTION_CONTEXT_H
#define PCB_ACTION_CONTEXT_H

#include "global.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \brief Action context holding shared state across actions
 *
 * This structure contains state that was previously maintained as
 * file-scoped static variables in action.c. Centralizing it here
 * makes it accessible to both C and C++ actions.
 */
typedef struct {
    /*!
     * \brief Mouse/selection state tracking
     *
     * Used by mouse event handlers, ActionMode, ActionDelete,
     * and Select(Convert) to track click positions and selected objects.
     */
    struct {
        Coord X, Y;              /**< Last mouse click position */
        Cardinal Buffer;         /**< Buffer number for copy/move */
        bool Click;              /**< Click event occurred */
        bool Moving;             /**< Selected object being moved */
        int Hit;                 /**< Type of object clicked */
        void *ptr1;              /**< Object pointer 1 */
        void *ptr2;              /**< Object pointer 2 */
        void *ptr3;              /**< Object pointer 3 */
    } Note;

    /*!
     * \brief Undo state tracking
     *
     * Used by ActionUndo to track the last layer when undoing line creation.
     */
    LayerType* lastLayer;

    /*!
     * \brief Line creation tracking
     *
     * Counts the number of lines added during multi-segment line creation.
     * Used by ActionUndo to properly undo multi-segment lines.
     */
    int addedLines;

    /*!
     * \brief Update deferral flags
     *
     * Used by ActionExecuteFile and ActionChangeName to defer screen updates
     * during batch operations for performance.
     */
    int defer_updates;
    int defer_needs_update;

    /*!
     * \brief Polygon editing state
     *
     * Tracks the last inserted point and current point index during
     * polygon/polygon-hole editing.
     */
    PointType InsertedPoint;
    Cardinal polyIndex;
    bool saved_mode;

    /*!
     * \brief Fake polygon/line for operations
     *
     * Used internally for certain polygon and line operations that need
     * temporary geometry.
     */
    struct {
        PolygonType *poly;
        LineType line;
    } fake;

#ifdef HAVE_LIBSTROKE
    /*!
     * \brief Stroke gesture state
     *
     * Tracks stroke gesture input if libstroke is available.
     */
    bool mid_stroke;
    BoxType StrokeBox;
#endif
} ActionContext;

/*!
 * \brief Global action context
 *
 * This is the single global instance of ActionContext used throughout
 * the application. Defined in action.c.
 */
extern ActionContext *pcb_action_context;

/*!
 * \brief Convenience macros for accessing action context
 *
 * These macros provide backward compatibility and cleaner syntax
 * when accessing the global action context.
 */
#define ACTION_CONTEXT      (pcb_action_context)
#define ACTION_NOTE         (pcb_action_context->Note)
#define ACTION_LAST_LAYER   (pcb_action_context->lastLayer)
#define ACTION_ADDED_LINES  (pcb_action_context->addedLines)
#define ACTION_DEFER        (pcb_action_context->defer_updates)
#define ACTION_DEFER_NEEDS  (pcb_action_context->defer_needs_update)

#ifdef __cplusplus
}
#endif

#endif /* PCB_ACTION_CONTEXT_H */
