/*
 * Valuations.cc
 *
 * Copyright (C) 2018 OpenCog Foundation
 *
 * Author: Nil Geisweiller
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "Valuations.h"

#include "XPatternMiner.h"

#include <opencog/util/Logger.h>
#include <opencog/util/oc_assert.h>
#include <opencog/atoms/base/Atom.h>
#include <opencog/atoms/pattern/PatternLink.h>
#include <opencog/atoms/core/LambdaLink.h>
#include <opencog/atomutils/TypeUtils.h>

#include <boost/range/algorithm/find.hpp>

namespace opencog
{

ValuationsBase::ValuationsBase(const Variables& vars) : variables(vars) {}

ValuationsBase::ValuationsBase() {}

bool ValuationsBase::novar() const
{
	return variables.empty();
}

Handle ValuationsBase::front_variable() const
{
	return variables.varseq[0];
}

Handle ValuationsBase::variable(unsigned i) const
{
	return variables.varseq[i];
}

SCValuations::SCValuations(const Variables& vars, const Handle& satset)
	: ValuationsBase(vars)
{
	if (satset)
	{
		OC_ASSERT(satset->get_type() == SET_LINK);
		for (const Handle& vals : satset->getOutgoingSet())
		{
			if (vars.size() == 1)
				values.push_back({vals});
			else
				values.push_back(vals->getOutgoingSet());
		}
	}
}

SCValuations SCValuations::erase_front() const
{
	return erase(front_variable());
}

SCValuations SCValuations::erase(const Handle& var) const
{
	// No variable, just return itself
	if (not variables.is_in_varset(var))
		return *this;

	// Remove variable
	Variables nvars(variables);
	nvars.erase(var);
	auto it = boost::find(variables.varseq, var);
	int dst = std::distance(variables.varseq.begin(), it);
	SCValuations nvals(nvars);
	if (not nvals.novar())
	{
		for (HandleSeq vals : values)
		{
			vals.erase(std::next(vals.begin(), dst));
			nvals.values.push_back(vals);
		}
	}
	return nvals;
}

bool SCValuations::operator<(const SCValuations& other) const
{
	return variables < other.variables;
}

Valuations::Valuations(const Handle& pattern, const HandleSet& texts)
	: ValuationsBase(XPatternMiner::get_variables(pattern))
{
	for (const Handle& cp : XPatternMiner::get_component_patterns(pattern))
	{
		Handle satset = XPatternMiner::restricted_satisfying_set(cp, texts);
		scvs.insert(SCValuations(XPatternMiner::get_variables(cp), satset));
	}
}

Valuations::Valuations(const Variables& vars, const SCValuationsSet& sc)
	: ValuationsBase(vars), scvs(sc) {}

Valuations::Valuations(const Variables& vars)
	: ValuationsBase(vars) {}

Valuations Valuations::erase_front() const
{
	Handle var = front_variable();
	Variables nvars(variables);
	nvars.erase(var);
	Valuations nvals(nvars);
	for (const SCValuations& scv : scvs)
	{
		SCValuations nscvals(scv.erase(var));
		if (not nscvals.novar())
			nvals.scvs.insert(nscvals);
	}
	return nvals;
}

const SCValuations& Valuations::get_scvaluations(const Handle& var) const
{
	for (const SCValuations& scv : scvs)
		if (scv.variables.is_in_varset(var))
			return scv;
	throw RuntimeException(TRACE_INFO, "There's likely a bug");
}

std::string oc_to_string(const SCValuations& scv)
{
	return oc_to_string(scv, "");
}

std::string oc_to_string(const SCValuationsSet& scvs, const std::string& indent)
{
	std::stringstream ss;
	ss << indent << "size = " << scvs.size() << std::endl;
	int i = 0;
	for (const auto& scv : scvs)
	{
		ss << indent << "scvaluations [" << i << "]:" << std::endl
		   << oc_to_string(scv, indent );
		++i;
	}
	return ss.str();
}

std::string oc_to_string(const SCValuationsSet& scvs)
{
	return oc_to_string(scvs, "");
}

std::string oc_to_string(const Valuations& valuations)
{
	return oc_to_string(valuations, "");
}

std::string oc_to_string(const HandleValuationsMap& h2vals, const std::string& indent)
{
	std::stringstream ss;
	ss << indent << "size = " << h2vals.size() << std::endl;
	int i = 0;
	for (const auto& hv : h2vals)
	{
		ss << indent << "atom [" << i << "]:" << std::endl
		   << oc_to_string(hv.first, indent);
		ss << indent << "valuations [" << i << "]:" << std::endl
		   << oc_to_string(hv.second, indent );
		++i;
	}
	return ss.str();
}

std::string oc_to_string(const HandleValuationsMap& h2vals)
{
	return oc_to_string(h2vals, "");
}

} // namespace opencog
