/*
 * Transport.cpp
 *
 *  Created on: May 24, 2022
 *      Author: naxo
 */

#include "Transport.h"

namespace spatial {

using namespace std;

ChannelTransport::ChannelTransport(Channel* chnl,string name,pattern::Mixture& mix,const yy::location& _loc) :
		Rule("Transport of mix by channel",mix,loc), channel(chnl) {

}

LinkTransport::LinkTransport(tuple<int,int,bool> link,string name,pattern::Mixture& mix,
		const yy::location& _loc) :
		simulation::Rule(name,mix,loc),
		srcCell(get<0>(link)),trgtCell(get<1>(link)),isBi(get<2>(link)) {
	script.emplace_back();
	script.front().t = pattern::ActionType::TRANSPORT;
	script.front().trgt_ag = ag_st_id(0,0);
}

} /* namespace spatial */
