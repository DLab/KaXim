/*
 * DataTable.h
 *
 *  Created on: Nov 9, 2021
 *      Author: naxo
 */

#ifndef SRC_DATA_STRUCTS_DATATABLE_H_
#define SRC_DATA_STRUCTS_DATATABLE_H_


#include <list>
#include <vector>
#include <Eigen/Dense>

#include <iostream>

namespace data_structs {

struct DataTable {
	//int cols,rows;
	std::list<std::string> row_names,col_names;
	Eigen::MatrixXd data;

	//const list<string> getColNames() const {
	//	return col_names;s
	//}

	DataTable() {};
	DataTable(const DataTable&) = default;
	template <typename DT> DataTable(const DT &rows,const DT &cols) :
		row_names(rows.begin(),rows.end()),
		col_names(cols.begin(),cols.end()),
		data(row_names.size(),col_names.size()){}

	template <typename DT> DataTable(DT& _data,bool row_index = false)
			: data(_data.size(),_data.front().size()-(row_index? 1:0)) {
		int i = 0;
		for(auto& row : _data){
			int j = row_index? -1:0;
			for(auto val : row){
				if(row_index && j != -1)
					data(i,j) = val;
				else{
					row_names.emplace_back(std::to_string(val));
					//std::cout << row_names.back() << " - " << std::to_string(val) << std::endl;
				}
				//std::cout << "(" << i << "," << j << ") = " << val << std::endl;
				j++;
			}
			i++;
		}
	}
	//virtual ~DataTable();
	//DataTable(const DataTable &other);
	//DataTable& operator=(const DataTable &other);

	const Eigen::MatrixXd& getData() const{
		return data;
	}

	std::string toString() const {
		std::string ret("Time");
		for(auto& col : col_names)
			ret += "\t" + col;
		ret += "\n";
		int i = 0;
		for(auto& row : row_names){
			std::stringstream ss;
			ss << data(i++,Eigen::indexing::all);
			ret += row +"\t"+ ss.str() + "\n";
		}
		return ret;
	}

};

} /* namespace data_structs */

#endif /* SRC_DATA_STRUCTS_DATATABLE_H_ */
