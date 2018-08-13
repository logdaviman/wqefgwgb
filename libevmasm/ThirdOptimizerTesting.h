/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <vector>
#include <libevmasm/AssemblyItem.h>
#include <libevmasm/Instruction.h>
#include <libevmasm/RuleList.h>
#include <libevmasm/SimplificationRule.h>
#include <tuple>
#include <boost/optional.hpp>

namespace dev
{
namespace solidity
{
using namespace dev;
using namespace solidity;
using namespace eth;
using namespace std;

class NewOptimizerPattern
{
public:
	enum class Kind { Operation, Constant, Any, Unknown };

public:
	NewOptimizerPattern(Instruction const& _instruction, vector<NewOptimizerPattern> const& _operands) : m_ptr(std::make_shared<Underlying>())
	{
		assertThrow(
			unsigned(instructionInfo(_instruction).args) == _operands.size(),
			OptimizerException,
			"number of operands passed does not match instruction"
		);

		m_ptr->m_kind = Kind::Operation;
		m_ptr->m_hasOperationValues = true;
		m_ptr->m_instruction = _instruction;
		m_ptr->m_operands = _operands;
	}

	NewOptimizerPattern(Instruction const& _instruction) : NewOptimizerPattern(_instruction, {}) {}

	NewOptimizerPattern(AssemblyItemType const& _type) : m_ptr(std::make_shared<Underlying>())
	{
		m_ptr->m_kind = Kind::Any;
		m_ptr->m_assemblyType = _type;
	}

	NewOptimizerPattern(u256 const& _constant) : m_ptr(std::make_shared<Underlying>())
	{
		m_ptr->m_kind = Kind::Constant;
		m_ptr->m_constant = _constant;
	}

	NewOptimizerPattern(unsigned const& _constant) : NewOptimizerPattern(u256(_constant)) {}

	NewOptimizerPattern(Kind const& _kind) : m_ptr(std::make_shared<Underlying>()) { m_ptr->m_kind = _kind; }

	Kind kind() const { return m_ptr->m_kind; }

	vector<NewOptimizerPattern> const& operands() const
	{
		assertThrow(hasOperationValues(), OptimizerException, "invalid request for operands");
		return m_ptr->m_operands;
	}

	vector<NewOptimizerPattern>& operands()
	{
		assertThrow(hasOperationValues(), OptimizerException, "invalid request for operands");
		return m_ptr->m_operands;
	}

	Instruction instruction() const
	{
		assertThrow(hasOperationValues(), OptimizerException, "invalid request for instruction");
		return m_ptr->m_instruction;
	}

	u256 constant() const
	{
		assertThrow(hasConstant(), OptimizerException, "invalid request for constant");
		return m_ptr->m_constant.get();
	}

	AssemblyItemType assemblyItemType() const
	{
		assertThrow(hasAssemblyItemType(), OptimizerException, "invalid request for assembly item type");
		return m_ptr->m_assemblyType.get();
	}

	bool matches(vector<AssemblyItem> const& _items, bool _unbind = true);

	u256 d() const
	{
		if (isBound())
		{
			if (kind() == Kind::Constant)
				return boundItems().back().data();
			assertThrow(false, OptimizerException, "invalid request for constant");
		}
		else
			return constant();
	}

	bool isBound() const { return m_ptr->m_boundItems.is_initialized(); }

	vector<AssemblyItem> const& boundItems() const
	{
		assertThrow(isBound(), OptimizerException, "invalid request for bound items");
		return m_ptr->m_boundItems.get();
	}

	bool hasOperationValues() const { return m_ptr->m_hasOperationValues; }
	bool hasConstant() const { return m_ptr->m_constant.is_initialized(); }
	bool hasAssemblyItemType() const { return m_ptr->m_assemblyType.is_initialized(); }

private:
	void bind(std::vector<AssemblyItem> const& _items);
	void unbind()
	{
		m_ptr->m_boundItems.reset();

		if (hasOperationValues())
		{
			for (auto& operand : operands()) operand.unbind();
		}
	}

	struct Underlying
	{
		boost::optional<std::vector<AssemblyItem>> m_boundItems;

		Kind m_kind;

		boost::optional<u256> m_constant;

		bool m_hasOperationValues = false;
		Instruction m_instruction;
		vector<NewOptimizerPattern> m_operands;

		boost::optional<AssemblyItemType> m_assemblyType;
	};

	std::shared_ptr<Underlying> m_ptr;
};

class ThirdOptimizer
{
public:
	vector<AssemblyItem> optimize(vector<AssemblyItem> const& _items);
private:
	void addDefaultRules();
	void addRules(vector<SimplificationRule<NewOptimizerPattern>> const& _rules);
	void addRule(SimplificationRule<NewOptimizerPattern> const& _rule);
	vector<SimplificationRule<NewOptimizerPattern>> m_rules;
};
}
}
