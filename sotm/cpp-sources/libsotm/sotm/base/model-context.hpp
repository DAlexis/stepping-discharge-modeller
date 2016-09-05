/*
 * global-context.hpp
 *
 *  Created on: 2 июня 2016 г.
 *      Author: dalexies
 */

#ifndef LIBSTEPMOD_STEPMOD_BASE_GLOBAL_CONTEXT_HPP_
#define LIBSTEPMOD_STEPMOD_BASE_GLOBAL_CONTEXT_HPP_

#include "sotm/base/transport-graph.hpp"
#include "sotm/base/time-iter.hpp"
#include "sotm/base/physical-payload.hpp"
#include <memory>

namespace sotm
{

class ModelContext : public IBifurcatable
{
public:
	void setNodePayloadFactory(std::unique_ptr<INodePayloadFactory> factory);
	void setLinkPayloadFactory(std::unique_ptr<ILinkPayloadFactory> factory);
	void setPhysicalContext(std::unique_ptr<IPhysicalContext> physicalContext);

	NodePayloadBase* createNodePayload(Node* node);
	LinkPayloadBase* createLinkPayload(Link* node);

	void doBranchingIteration(double dt);

	void doBifurcation(double time, double dt) override final;

	inline IPhysicalContext* physicalContext() { return m_physicalContext.get(); }

	PhysicalPayloadsRegister payloadsRegister;
	GraphRegister graphRegister;

private:
	void branchIteration(double dt, Node* node);
	void discreteStep(double dt, Node* node);
	std::unique_ptr<INodePayloadFactory> m_nodePayloadFactory;
	std::unique_ptr<ILinkPayloadFactory> m_linkPayloadFactory;
	std::unique_ptr<IPhysicalContext> m_physicalContext;
};

}







#endif /* LIBSTEPMOD_STEPMOD_BASE_GLOBAL_CONTEXT_HPP_ */
