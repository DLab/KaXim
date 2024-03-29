/*
 * Compartment.cpp
 *
 *  Created on: Sep 7, 2016
 *      Author: naxo
 */


#include "../spatial/Compartment.h"

#include "../util/Exceptions.h"

namespace pattern {

/*****************************************
 *********** Class Compartment ***********
 ****************************************/
unsigned int Compartment::TOTAL_CELLS = 0;
unsigned int Compartment::getTotalCells(){
	return TOTAL_CELLS;
}
Compartment::Compartment(const std::string &nme)
		:name(nme),dimensions(1,1),volume(nullptr) {//TODO initialize dims with (1,1)??
	firstCell = TOTAL_CELLS;
	cellsCount = 1;
	TOTAL_CELLS += 1;
}

Compartment::~Compartment() {
	delete volume;
}

void Compartment::setDimensions(const std::vector<short> &dims){
	//TODO first cell?
	TOTAL_CELLS -= cellsCount;
	dimensions = dims;
	cellsCount = 1;
	for(unsigned int i = 0; i < dimensions.size(); i++)
		cellsCount *= dimensions[i];
	TOTAL_CELLS += cellsCount;
}
void Compartment::setVolume(const state::BaseExpression *vol){
	if(volume)
		delete(volume);
	volume = vol;
}

const std::string& Compartment::getName() const {
	return name;
}
const state::BaseExpression& Compartment::getVolume() const{
	return *volume;
}

void Compartment::getCellIndex(int cell_id,std::vector<short>& index) const{
	if(index.size() != dimensions.size())
		index = std::vector<short>(dimensions.size());
	cell_id -= firstCell;
	if(cell_id < 0 || cell_id >= cellsCount)
		throw std::invalid_argument("The cell-id "+std::to_string(cell_id)+
				" is not in compartment "+name+".");;
	int factor = cellsCount;
	unsigned i = dimensions.size();
	while(i--){
		factor /= dimensions[i];
		index[i] = cell_id/factor;
		cell_id %= factor;
	}
}

int Compartment::getCellId(const std::vector<short> &cell_index) const{
	int factor = 1, id = 0;
	for(unsigned int i = 0; i < dimensions.size(); i++){
		id += factor*cell_index[i];
		factor *= dimensions[i];
	}
	return id+firstCell;
}

const std::vector<short>& Compartment::getDimensions() const{
	return dimensions;
}

bool Compartment::nextCell(std::vector<short>& cell) const {
	for(unsigned int i = 0; i < dimensions.size(); i++)
		if(cell[i] < dimensions[i]-1){
			cell[i]++;
			return true;
		}
		else{
			cell[i] = 0;
		}
	return false;
}

int Compartment::getLastCellId() const {
	return firstCell+cellsCount;
}
int Compartment::getFirstCellId() const {
	return firstCell;
}


using namespace boost::numeric::ublas;

//needed for CompExpression::solve()
 /* Matrix inversion routine.
 Uses lu_factorize and lu_substitute in uBLAS to invert a matrix */
template<class T>
bool InvertMatrix(const matrix<T>& input, matrix<T>& inverse);


/************** CompartmentExpr ***********/

CompartmentExpr::CompartmentExpr(const Compartment& c,const std::list<const state::BaseExpression*> &expr)
	: comp(c),cellExpr(expr),b(expr.size())
{
	if(c.getDimensions().size() < cellExpr.size())
		throw invalid_argument("Compartment "+comp.getName()+" has not enough dimensions.");
}


void CompartmentExpr::cellIds(std::list<int>& cell_list,
		std::list<short> *cell_values,std::vector<short> &cell_index,unsigned int dim) const{
	if(dim == comp.getDimensions().size())
		cell_list.push_back(comp.getCellId(cell_index));
	else {
		for(std::list<short>::iterator it = cell_values[dim].begin(); it != cell_values[dim].end();it++){
			cell_index[dim] = *it;
			cellIds(cell_list,cell_values,cell_index,dim+1);
		}
	}
}

const Compartment& CompartmentExpr::getCompartment() const{
	return comp;
}

std::list<int> CompartmentExpr::getCells(const AuxNames& aux_values,const simulation::SimContext& context) const{
	std::list<int> ret;
	unsigned int dim = 0;
	std::list<short> *cell_values = new std::list<short>[comp.getDimensions().size()];
	//iteration in cell index expressions
	for(std::list<const state::BaseExpression*>::const_iterator it = cellExpr.cbegin();it != cellExpr.cend();it++){
		try{
			int index = (*it)->getValue(context).iVal;//TODO use real VarVector
			if(index < 0 || index >= comp.getDimensions()[dim])
				return ret;
			cell_values[dim].push_back(index);
		}
		catch(const std::out_of_range &e){
			std::unordered_map<std::string,FL_TYPE> factors;
			auto b = (*it)->auxFactors(factors);
			for(auto name_val : aux_values){
				//maybe if factors[s] = 0 for none then remove '?' TODO
				b -= name_val.second * (factors.count(name_val.first) ? factors[name_val.first] : 0);
				factors.erase(name_val.first);
			}
			if(factors.size() > 1)
				throw std::exception();//Cannot solve this equation
			for(int i = 0; i < comp.getDimensions()[dim];i++){
				auto value = (b + i) / (FL_TYPE)factors.begin()->second;
				if(value == (int)value)
					cell_values[dim].push_back(i);
			}
		}
		catch(std::exception &e){
			std::cout << e.what() << std::endl;
		}
		dim++;
	}
	//fill dimension without an explicit expression in indexlist
	for(unsigned int i = dim; i < comp.getDimensions().size(); i++)
		for(short index = 0; index < comp.getDimensions()[i]; index++)
			cell_values[i].push_back(index);

	//short *cell_index = new short[comp.getDimensions().size()];
	std::vector<short> cell_index(comp.getDimensions().size());

	cellIds(ret,cell_values,cell_index,0);
	delete[] cell_values;
	return ret;
}

namespace boost_ublas = boost::numeric::ublas;

void CompartmentExpr::solve(const std::vector<short> &cell_index,AuxNames& aux_values) const{
	boost_ublas::vector<short,std::vector<short> > cell(cellExpr.size(),cell_index);
	boost_ublas::vector<FL_TYPE> b_;
	if(cellExpr.size() != inverseA.size1())
		b_ = boost_ublas::prod(transA,b+cell);
	else
		b_ = (b + cell);
	boost_ublas::vector<FL_TYPE> result = boost_ublas::prod(inverseA,b_);
	auto v1 = (b+cell);
	auto v2 = boost_ublas::prod(A,result);
	for(unsigned int i = 0; i < v1.size(); i++)
		if(  v1(i) != v2(i) )
			throw std::invalid_argument("approximated solution");
	for(unsigned int i = 0; i < result.size(); i++)
		if(result[i] == (int)result[i])
			try{//check whether var[i] has a previous value.
				if(aux_values.at(varOrder[i]) != result[i])
					throw std::invalid_argument("var "+varOrder[i]+" has a previous fixed value.");
			}
			catch(std::out_of_range &e){
				aux_values[varOrder[i]] = result[i];
			}
		else //var i has no an integer value
			throw std::invalid_argument("var "+varOrder[i]+" has not an integer value");

}

void CompartmentExpr::setEquation(){
	//get factors, initialize b and var_order
	std::set<std::string> var_names;
	int j = 0;
	auto *var_factors = new std::unordered_map<std::string,FL_TYPE>[cellExpr.size()];
	for(std::list<const state::BaseExpression*>::const_iterator it = cellExpr.cbegin();it != cellExpr.cend();it++){
		b[j] = -(*it)->auxFactors(var_factors[j]);
		for(auto var_value : var_factors[j])
			var_names.insert(var_value.first);
		j++;
	}
	varOrder = std::vector<std::string>(var_names.begin(),var_names.end());
	//initialize A
	j = 0;
	auto A = matrix<FL_TYPE> (cellExpr.size(),varOrder.size());
	for(std::list<const state::BaseExpression*>::const_iterator it = cellExpr.cbegin();it != cellExpr.cend();it++){
		for(unsigned int i = 0; i < varOrder.size(); i++)
			A(j,i) = var_factors[j][varOrder[i]];
		j++;
	}
	this->A = A;
	if(A.size1() > A.size2()){//if there are more equations than vars
		transA = matrix<FL_TYPE>(trans(A));
		A = prod(transA,A);
		//b = prod(At,b);
	}
	else if (A.size1() < A.size2())//if there are more vars than eqs
		throw invalid_argument("Cannot solve the implicit equation system for this expression.");

	inverseA = matrix<FL_TYPE>(A.size1(),A.size2());
	bool invertible = InvertMatrix(A,inverseA);
	if(!invertible)
		throw invalid_argument("Cannot solve the implicit equation system for this expression.");
}


template<class T>
bool InvertMatrix(const matrix<T>& input, matrix<T>& inverse)
{
	typedef permutation_matrix<std::size_t> pmatrix;

	// create a working copy of the input
	matrix<T> A(input);

	// create a permutation matrix for the LU-factorization
	pmatrix pm(A.size1());

	// perform LU-factorization
	int res = lu_factorize(A, pm);
	if (res != 0)
		return false;

	// create identity matrix of "inverse"
	inverse.assign(identity_matrix<T> (A.size1()));

	// backsubstitute to get the inverse
	lu_substitute(A, pm, inverse);

	return true;
}





//DEBUG methods
std::string Compartment::toString() const {
	std::string ret(name);
	for(unsigned int i = 0; i < dimensions.size(); i++)
		ret += "["+std::to_string(dimensions[i])+"]";
	return ret;
}
std::string Compartment::cellToString(const std::vector<short>& cell){
	std::string ret;
	for(unsigned i = 0; i < cell.size(); i++)
		ret = ret + "[" + std::to_string(cell[i]) + "]";
	return ret;
}
std::string Compartment::cellIdToString(int cell_id) const{
	std::vector<short> cell;
	getCellIndex(cell_id,cell);
	return name+cellToString(cell);
}

std::string CompartmentExpr::toString() const {
	std::string ret(comp.getName());
	for(std::list<const state::BaseExpression*>::const_iterator it = cellExpr.cbegin(); it != cellExpr.cend(); it++)
		break;//ret += it->toString();
	return ret;
}


/************ class UseExpr *****************/

std::set<int> UseExpression::ALL_CELLS;
//TODO filter
UseExpression::UseExpression(size_t comps_count,const state::BaseExpression* where):
		filter(dynamic_cast<const state::AlgExpression<bool>* >(where) ),isComplete(false) {
	if(comps_count > 0){
		reserve(comps_count);
		cells = new std::set<int>();
	}
	else{
		if(ALL_CELLS.size() == 0)
			for(unsigned i = 0; i < Compartment::getTotalCells(); i++)
				ALL_CELLS.insert(i);
		cells = &ALL_CELLS;
		isComplete = true;
	}
}
//TODO filter
UseExpression::~UseExpression(){
	if(size())
		delete cells;
	if(filter)
		delete filter;
}

void UseExpression::evaluateCells(const simulation::SimContext& context,
		UseExpression::iterator expr_it,
		const AuxNames aux_values){
	if(expr_it == UseExpression::iterator())
		expr_it = begin();
	if(expr_it == end())
		return;
	auto& expr = *expr_it;
	expr.setEquation();
	std::vector<short> cell_index(expr.getCompartment().getDimensions().size());
	for(auto cell_id : expr.getCells(aux_values,context)){
		if(cells->find(cell_id)!= cells->end())
			continue;
		auto aux_buff(aux_values);
		expr.getCompartment().getCellIndex(cell_id,cell_index);
		try{
			expr.solve(cell_index,aux_buff);//throws
		}
		catch(std::invalid_argument &e){
			//std::cout << e.what() << std::endl;
			continue;
		}
		evaluateCells(context,expr_it+1,aux_buff);
		cells->insert(cell_id);
	}

	if(expr_it == begin()){
		isComplete = true;
	}
}


const std::set<int>& UseExpression::getCells() const {
	if(!isComplete)
		throw std::invalid_argument("Cannot call getCells() in a not evaluated use expression.");
	return *cells;
}

} /* namespace pattern */
