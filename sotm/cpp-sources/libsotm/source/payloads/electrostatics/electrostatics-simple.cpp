#include "sotm/payloads/electrostatics/electrostatics-simple.hpp"
#include "sotm/base/model-context.hpp"
#include "sotm/utils/const.hpp"
#include "sotm/math/distrib-gen.hpp"

#include <iostream>

#include <ios>
#include <iomanip>
#include <sstream>

using namespace std;

using namespace sotm;

double ElectrostaticNodePayload::chargeMin = 0.0;
double ElectrostaticNodePayload::chargeMax = 10e-9;

void ElectrostaticPhysicalContext::destroyGraph()
{
	m_readyToDestroy = true;
}

bool ElectrostaticPhysicalContext::readyToDestroy()
{
	return m_readyToDestroy;
}

void ElectrostaticPhysicalContext::calculateSecondaryValues()
{
}

void ElectrostaticPhysicalContext::calculateRHS(double time)
{
	UNUSED_ARG(time);
}

void ElectrostaticPhysicalContext::addRHSToDelta(double m)
{
	UNUSED_ARG(m);
}

void ElectrostaticPhysicalContext::makeSubIteration(double dt)
{
	UNUSED_ARG(dt);
}

void ElectrostaticPhysicalContext::step()
{

}

void ElectrostaticPhysicalContext::setDischargeFunc(Function1D func)
{
	m_dischargeProb = func;
	m_integralOfProb.reset(new DefinedIntegral(m_dischargeProb, -20e6, 20e6, 10000));
}

void ElectrostaticPhysicalContext::setExternalConstField(Vector<3> field)
{
	m_externalConstField = field;
}

void ElectrostaticPhysicalContext::getElectricField(const Vector<3>& point, Vector<3>& outField, double& outPotential, const Node* exclude)
{
	outPotential = - (point ^ m_externalConstField);
	outField = m_externalConstField;

	GraphRegister::NodeVisitor nodeVisitor = [&point, exclude, &outPotential, &outField](const Node* node) {

		// Skip this node
		if (node == exclude)
			return;
		Vector<3> r1 = node->pos;

		double dist = (point - r1).len();
		double dist3 = dist*dist*dist;

		double charge = static_cast<ElectrostaticNodePayload*>(node->payload.get())->charge.current;

		outPotential += Const::Si::k * charge / dist;

		outField[0] += Const::Si::k * charge * (point[0]-r1[0]) / dist3;
		outField[1] += Const::Si::k * charge * (point[1]-r1[1]) / dist3;
		outField[2] += Const::Si::k * charge * (point[2]-r1[2]) / dist3;
	};

	m_model->graphRegister.applyNodeVisitorWithoutGraphChganges(nodeVisitor);
}

////////////////////////////////////
////////////////////////////////////
// ElectrostaticNodePayload

ElectrostaticNodePayload::ElectrostaticNodePayload(PhysicalPayloadsRegister* reg, Node* node) :
		NodePayloadBase(reg, node)
{
}

void ElectrostaticNodePayload::calculateSecondaryValues()
{
	static_cast<ElectrostaticPhysicalContext*>(node->physicalContext())->
			getElectricField(node->pos, externalField, phi, this->node.data());


	double capacity = radius / Const::Si::k;
	externalField = externalField * 3;
	phi += charge.current / capacity;
}

void ElectrostaticNodePayload::calculateRHS(double time)
{
	charge.rhs = 0;
	Node::LinkVisitor linkVisitor = [this](Link* link, LinkDirection dir)
	{
		double current = static_cast<ElectrostaticLinkPayload*>(link->payload.get())->getCurrent();
		charge.rhs += current * (dir == LinkDirection::in ? 1.0 : -1.0);
	};
	node->applyConnectedLinksVisitor(linkVisitor);
}

void ElectrostaticNodePayload::addRHSToDelta(double m)
{
	charge.addRHSToDelta(m);
}

void ElectrostaticNodePayload::makeSubIteration(double dt)
{
	charge.makeSubIteration(dt);
}

void ElectrostaticNodePayload::step()
{
	charge.step();
}

void ElectrostaticNodePayload::doBifurcation(double time, double dt)
{
	// Check if physics tells us we can release parent object
	ElectrostaticPhysicalContext* context = ElectrostaticPhysicalContext::cast(node->context()->physicalContext());
	if (context->readyToDestroy())
	{
		onDeletePayload();
	}
}

void ElectrostaticNodePayload::getBranchingParameters(double time, double dt, BranchingParameters& branchingParameters)
{
	ElectrostaticPhysicalContext* context = static_cast<ElectrostaticPhysicalContext*>(node->physicalContext());
	double E1 = Const::Si::k*charge.current / sqr(radius);
	DistributionResult<SphericalPoint> res = generateDischargeDirection(
			dt,
			radius,
			externalField.len(),
			E1,
			context->m_dischargeProb,
			*(context->m_integralOfProb.get())
	);
	branchingParameters.needBranching = res.isHappened;
	if (branchingParameters.needBranching)
	{
		SphericalVectorPlacer pl(externalField);
		branchingParameters.direction = pl.place(1.0, res.value);

		Vector<3> branchStartPoint = node->pos + branchingParameters.direction * (radius*1.00);
		double len = calculateBranchLen(branchStartPoint, branchingParameters.direction, 0.5, 0.3);
		cout << "branch len should be " << len << endl;

		len = 0.3;

		Vector<3> newPlace = node->pos + branchingParameters.direction * len;
		Node *nearest = context->m_model->graphRegister.getNearestNode(newPlace);
		if (nearest && (nearest->pos - newPlace).len() < radius*2.0)
		{
			cout << "Branching disabled" << endl;
			branchingParameters.needBranching = false;
		}

		branchingParameters.length = len;
		cout << "Branching" << endl;
	}
}

void ElectrostaticNodePayload::getColor(double* rgb)
{
	double v = (charge.previous - chargeMin) / (chargeMax - chargeMin);
	if (v < 0.0) v = 0.0;
	if (v > 1.0) v = 1.0;

	rgb[0] = v;
	rgb[1] = 0;
	rgb[2] = 1.0-v;
}

double ElectrostaticNodePayload::getSize()
{
	return radius;
}

std::string ElectrostaticNodePayload::getFollowerText()
{
	std::ostringstream ss;
	ss << "  q = " << std::scientific << std::setprecision(2) << charge.current << endl;
	ss << "  phi = " << phi;
	return ss.str();
}

void ElectrostaticNodePayload::setCharge(double charge)
{
	this->charge.setInitial(charge);
}

double ElectrostaticNodePayload::calculateBranchLen(const Vector<3>& startPoint, const Vector<3>& direction, double eDiffMax, double lenMax)
{

	ElectrostaticPhysicalContext* context = static_cast<ElectrostaticPhysicalContext*>(node->physicalContext());

	Vector<3> step = direction;
	step.normalize();
	step = step * branchProbeStep;

	Vector<3> currentPoint = startPoint;
	double currentPhi = 0;
	Vector<3> startField;
	Vector<3> currentField;

	Vector<3> deltaField;

	// Getting start field
	context->getElectricField(currentPoint, startField, currentPhi);

	double len = 0;

	do {
		context->getElectricField(currentPoint, currentField, currentPhi);
		currentPoint += step;
		len = (currentPoint - startPoint).len();
		deltaField = currentField - startField;

	} while (deltaField.len() / startField.len() < eDiffMax && len < lenMax);

	return len;
}

void ElectrostaticNodePayload::setChargeColorLimits(double chargeMin, double chargeMax)
{
	ElectrostaticNodePayload::chargeMin = chargeMin;
	ElectrostaticNodePayload::chargeMax = chargeMax;
}

////////////////////////////////////
////////////////////////////////////
// ElectrostaticLinkPayload

ElectrostaticLinkPayload::ElectrostaticLinkPayload(PhysicalPayloadsRegister* reg, Link* link) :
		LinkPayloadBase(reg, link)
{
}


void ElectrostaticLinkPayload::calculateSecondaryValues()
{
}

void ElectrostaticLinkPayload::calculateRHS(double time)
{
	heatness.rhs =
			sqr(getCurrent()) / conductivity // Heat source
			- 0.5*heatness.rhs; // Dissipation
}

void ElectrostaticLinkPayload::addRHSToDelta(double m)
{
	heatness.addRHSToDelta(m);
}

void ElectrostaticLinkPayload::makeSubIteration(double dt)
{
	heatness.makeSubIteration(dt);
}

void ElectrostaticLinkPayload::step()
{
	heatness.step();
}

void ElectrostaticLinkPayload::doBifurcation(double time, double dt)
{
	double current = getCurrent();
	if (current != 0.0 && fabs(current) < 5e-7)
	{
		onDeletePayload();
		return;
	}

	// Check if physics tells us we can release parent object
	ElectrostaticPhysicalContext* context = ElectrostaticPhysicalContext::cast(link->context()->physicalContext());
	if (context->readyToDestroy())
	{
		onDeletePayload();
	}
}

void ElectrostaticLinkPayload::init()
{
	setTemperature(300);
}

std::string ElectrostaticLinkPayload::getFollowerText()
{
	std::ostringstream ss;
	ss << "  t = " << std::scientific << std::setprecision(2) << getTemperature() << endl;
	ss << "  I = " << std::scientific << std::setprecision(2) << getCurrent() << endl;
	return ss.str();
}

double ElectrostaticLinkPayload::getCurrent()
{
	Node* n1 = link->getNode1();
	ElectrostaticNodePayload* p1 = static_cast<ElectrostaticNodePayload*>(n1->payload.get());

	Node* n2 = link->getNode2();
	ElectrostaticNodePayload* p2 = static_cast<ElectrostaticNodePayload*>(n2->payload.get());

	return (p1->phi - p2->phi) * conductivity;
}

void ElectrostaticLinkPayload::setTemperature(double temp)
{
	heatness.setInitial(temp*heatCapacity());
}

double ElectrostaticLinkPayload::getTemperature()
{
	return heatness.current / heatCapacity();
}

double ElectrostaticLinkPayload::heatCapacity()
{
	double l = link->lengthCached();
	double volume = Const::pi/4.0*radius*radius*l;
	double heatCapacity = volume * Const::Si::SpecificHeat::air;
	return heatCapacity;
}
