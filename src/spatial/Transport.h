/*
 * Transport.h
 *
 *  Created on: May 24, 2022
 *      Author: naxo
 */

#ifndef SRC_SPATIAL_TRANSPORT_H_
#define SRC_SPATIAL_TRANSPORT_H_

#include "../simulation/Rule.h"
#include "Channel.h"

namespace spatial {

class ChannelTransport: public simulation::Rule {
	Channel* channel;

public:
	ChannelTransport(Channel* chnl,std::string name,pattern::Mixture& mix,const yy::location& _loc);
	//virtual ~ChannelTransport();
};

class LinkTransport: public simulation::Rule {
	int srcCell,trgtCell;
	bool isBi;

public:
	LinkTransport(std::tuple<int,int,bool> link,std::string name,pattern::Mixture& mix,
			const yy::location& _loc);
	//virtual ~LinkTransport();

	virtual int getTargetCell(int cell_id) const override {
		return cell_id == srcCell ? trgtCell : (cell_id == trgtCell && isBi ? srcCell : -1);
				//throw std::invalid_argument("LinkTransport::getTargetCell(): invalid source cell id."));
	}
};

} /* namespace spatial */

#endif /* SRC_SPATIAL_TRANSPORT_H_ */
