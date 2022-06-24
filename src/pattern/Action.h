/*
 * Action.h
 *
 *  Created on: May 25, 2020
 *      Author: naxo100
 */

#ifndef SRC_PATTERN_ACTION_H_
#define SRC_PATTERN_ACTION_H_

#include "../util/params.h"
#include "../expressions/BaseExpression.h"

namespace pattern {


enum ActionType {
	CHANGE,
	ASSIGN,
	LINK,
	UNBIND,
	CREATE,//id_rhs,id_ag_mix
	DELETE,//id_lhs
	TRANSPORT,//id_lhs,id_trgt_comp
	ERROR
};

/** \brief Target mix_agent and the action to be applied. */
struct Action {
	ActionType t;		///< type of action
	ag_st_id trgt_ag;	///< target agent-coords || new-ag pos
	int trgt_st;		///< target-site || transport target-cell
	bool is_new;		///< true if the action is on a new agent
	bool side_eff;		///< true when this action will induce side effects
	small_id new_label;	///< new CHANGE value
	const expressions::BaseExpression* new_value;	///< new ASSIGN value
	const Action* chain;///< Another action chained with this one (used for BIND).
	//enum {NEW=1,S_EFF=2};
};

}

#endif /* SRC_PATTERN_ACTION_H_ */
