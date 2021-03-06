#ifndef BUILDER_DYN_VAR_H
#define BUILDER_DYN_VAR_H

#include "builder/builder.h"
namespace builder {

class var {
public:
	// Optional var name
	std::string var_name;
	block::var::Ptr block_var;
	block::decl_stmt::Ptr block_decl_stmt;

	static block::type::Ptr create_block_type(void) {
		// Cannot create block type for abstract class
		assert(false);
	}

	var() = default;

	explicit operator bool();

	// This is for enabling dynamic inheritance
	virtual ~var() = default;


};

template<typename T, typename MT, typename BT>
class dyn_var_base: public var, public MT {
public:

	typedef dyn_var_base<T, MT, BT> my_type;
	typedef BT associated_BT;
	typedef T stored_type;
	typedef my_type super;
		
	template <typename... types>
	BT operator()(const types &... args) {
		return ((BT) * this)(args...);
	}

	// Required for the MT's virtual function if it exists
	virtual block::expr::Ptr get_parent() const {
		return ((BT) (*this)).get_parent();
	}
	
	// These three need to be defined inside the class, cannot be defined globally
	BT operator=(const var &a) { 
		return (BT) * this = a; 
	}

	BT operator[] (const BT &a) {
		return ((BT) *this)[a];
	}
	BT operator=(const BT &a) {
		return (BT)*this = a;
	}

	BT operator=(const dyn_var_base<T, MT, BT> &a) {
		return (BT) * this = a;
	}

	template <typename TO>
	BT operator=(const dyn_var_base<TO, MT, BT> &a) {
		return (BT) * this = a;
	}

	BT operator=(const int &a) { 
		return operator=((BT)a); 
	}

	BT operator=(const double &a) { 
		return operator=((BT)a); 
	}

	template <typename Ts>
	BT operator=(const static_var<Ts> &a) {
		return operator=((BT)a);
	}
	BT operator=(const member_base &a) {
		return operator=((BT)a);
	}

	BT operator!() { return !(BT) * this; }
	operator bool() { return (bool)(BT) * this; }

	static block::type::Ptr create_block_type(void) { 
		return type_extractor<T>::extract_type(); 
	}

	void create_dyn_var(bool create_without_context = false) {
		if (create_without_context) {
			block::var::Ptr dyn_var = std::make_shared<block::var>();
			dyn_var->var_type = create_block_type();
			block_var = dyn_var;
			return;
		}
		assert(builder_context::current_builder_context != nullptr);
		assert(builder_context::current_builder_context->current_block_stmt != nullptr);
		builder_context::current_builder_context->commit_uncommitted();
		block::var::Ptr dyn_var = std::make_shared<block::var>();
		dyn_var->var_type = create_block_type();
		block_var = dyn_var;
		tracer::tag offset = get_offset_in_function();
		dyn_var->static_offset = offset;
		block_decl_stmt = nullptr;
		if (builder_context::current_builder_context->bool_vector.size() > 0)
			return;
		block::decl_stmt::Ptr decl_stmt = std::make_shared<block::decl_stmt>();
		decl_stmt->static_offset = offset;
		decl_stmt->decl_var = dyn_var;
		decl_stmt->init_expr = nullptr;
		block_decl_stmt = decl_stmt;
		builder_context::current_builder_context->add_stmt_to_current_block(decl_stmt);
	}
	// Basic and other constructors
	dyn_var_base(const char* name=nullptr) { 
		if (builder_context::current_builder_context == nullptr) {
			create_dyn_var(true); 
			if (name != nullptr) {
				block_var->var_name = name;
				var_name = name;
			}
		} else
			create_dyn_var(false); 
	}
	dyn_var_base(const dyn_var_sentinel_type& a, std::string name = "") {
		create_dyn_var(true);
		if (name != "") {
			block_var->var_name = name;
			var_name = name;
		}
	
	}
	// A very special move constructor that is used to create exact 
	// replicas of variables
	dyn_var_base(const dyn_var_consume &a) {
		block_var = a.block_var;
		var_name = block_var->var_name;
		block_decl_stmt = nullptr;	
	}

	dyn_var_base(const my_type &a) : my_type((BT)a) {}

	
	template <typename TO>
	dyn_var_base(const dyn_var_base<TO, MT, BT> &a) : my_type((BT)a) {}
	
	template <typename TO>
	dyn_var_base(const static_var<TO> &a) : my_type((TO)a) {}

	dyn_var_base(const BT &a) {
		builder_context::current_builder_context->remove_node_from_sequence(a.block_expr);
		create_dyn_var();
		if (builder_context::current_builder_context->bool_vector.size() > 0)
			return;
		block_decl_stmt->init_expr = a.block_expr;
	}

	dyn_var_base(const int &a) : my_type((BT)a) {}
	dyn_var_base(const bool &a) : my_type((BT)a) {}
	dyn_var_base(const double &a) : my_type((BT)a) {}
	dyn_var_base(const float &a) : my_type((BT)a) {}
	dyn_var_base(const member_base& a): my_type((BT)a) {}

	dyn_var_base(const std::initializer_list<BT> &_a) {
		std::vector<BT> a(_a);

		assert(builder_context::current_builder_context != nullptr);
		for (unsigned int i = 0; i < a.size(); i++) {
			builder_context::current_builder_context->remove_node_from_sequence(a[i].block_expr);
		}
		create_dyn_var();
		if (builder_context::current_builder_context->bool_vector.size() > 0)
			return;

		tracer::tag offset = get_offset_in_function();
		block::initializer_list_expr::Ptr list_expr = std::make_shared<block::initializer_list_expr>();
		list_expr->static_offset = offset;
		for (unsigned int i = 0; i < a.size(); i++) {
			list_expr->elems.push_back(a[i].block_expr);
		}
		block_decl_stmt->init_expr = list_expr;
	}

	virtual ~dyn_var_base() = default;

	dyn_var_base* addr(void) {
		return this;
	}

};

template<typename BT>
builder_base<BT>::builder_base(const var &a) {
	assert(builder_context::current_builder_context != nullptr);
	block_expr = nullptr;
	if (builder_context::current_builder_context->bool_vector.size() > 0)
		return;
	
	tracer::tag offset = get_offset_in_function();
	assert(a.block_var != nullptr);
	block::var_expr::Ptr var_expr = std::make_shared<block::var_expr>();
	var_expr->static_offset = offset;

	var_expr->var1 = a.block_var;
	builder_context::current_builder_context->add_node_to_sequence(
	    var_expr);

	block_expr = var_expr;
}


template <typename T>
typename std::enable_if<std::is_base_of<var, T>::value>::type create_return_stmt(const T &a) {
	create_return_stmt((typename T::associated_BT)a);	
}


}

#endif
