/*
 * electrostatics-simple.hpp
 *
 *  Created on: 13 дек. 2016 г.
 *      Author: dalexies
 */

#ifndef LIBSOTM_SOTM_PAYLOADS_ELECTROSTATICS_ELECTROSTATICS_SIMPLE_HPP_
#define LIBSOTM_SOTM_PAYLOADS_ELECTROSTATICS_ELECTROSTATICS_SIMPLE_HPP_

#include "sotm/base/physical-payload.hpp"
#include "sotm/math/integration.hpp"
#include "sotm/output/variables.hpp"

#include <memory>

namespace sotm
{

class ElectrostaticPhysicalContext : public PhysicalContextBase
{
friend class ElectrostaticNodePayload;
public:
	void destroyGraph();
	bool readyToDestroy();

	virtual void calculateSecondaryValues(double time) override;
	virtual void calculateRHS(double time) override;
	virtual void addRHSToDelta(double m) override;
	virtual void makeSubIteration(double dt) override;
	virtual void step() override;

	void doBifurcation(double time, double dt) { UNUSED_ARG(time); UNUSED_ARG(dt); }

	void setDischargeFunc(Function1D func);
	void setExternalConstField(Vector<3> field);

	void getElectricField(const Vector<3>& point, Vector<3>& outField, double& outPotential, const Node* excludeNode = nullptr);

	bool testConnection(Node* n1, Node* n2);

	static inline ElectrostaticPhysicalContext* cast(IPhysicalContext* context)
	{
		return static_cast<ElectrostaticPhysicalContext*>(context);
	}

	static inline const ElectrostaticPhysicalContext* cast(const IPhysicalContext* context)
	{
		return static_cast<const ElectrostaticPhysicalContext*>(context);
	}


	Parameter<double> airTemperature = 300;

	Parameter<double> branchingStep = 0.3;

	Parameter<bool>   smartBranching = false;
	Parameter<double> smartBranchingEDiff = 0.5;
	Parameter<double> smartBranchingMaxLen = 0.5;

	Parameter<double> initialConductivity = 1e-5;
	Parameter<double> minimalConductivity = initialConductivity*0.95;
	Parameter<double> minimalCurrent = 0.0;//3e-7;

	Parameter<double> connectionCriticalField;

	Parameter<double> nodeRadius = 0.13;

	Parameter<double> linkEta = 1e-4;
	Parameter<double> linkBeta = 1e4;

	Scaler chargeScaler;
	LinearGradientColorMapper chargeColorMapper;

	Scaler conductivityScaler;
	LinearGradientColorMapper conductivityColorMapper;

private:
	bool m_readyToDestroy = false;
	Function1D m_dischargeProb{zero};
	std::unique_ptr<DefinedIntegral> m_integralOfProb;
	Vector<3> m_externalConstField;
};

class ElectrostaticNodePayload : public NodePayloadBase
{
public:
	ElectrostaticNodePayload(PhysicalPayloadsRegister* reg, Node* node);

	SOTM_INLINE ElectrostaticPhysicalContext* context() { return static_cast<ElectrostaticPhysicalContext*>(node->physicalContext()); }

	void calculateSecondaryValues(double time) override;
	void calculateRHS(double time) override;
	void addRHSToDelta(double m) override;
	void makeSubIteration(double dt) override;
	void step() override;
	void prepareBifurcation(double time, double dt) override;
	void doBifurcation(double time, double dt) override;
	void init() override;
	void getBranchingParameters(double time, double dt, BranchingParameters& branchingParameters) override;

	void getColor(double* rgb) override;
	double getSize() override;
	std::string getFollowerText() override;

	void setCharge(double charge);

	static void setChargeColorLimits(double chargeMin, double chargeMax);
	// Parameters
	double branchProbeStep = 0.001;

	// Primary
	Variable charge;

	// Secondary
	double phi = 0; // Electrostatic potential
	Vector<3> externalField; // Electric field from other nodes

private:
	void calculateExtFieldAndPhi();
	void findTargetToConnect();
	void connectToTarget();
	double calculateBranchLen(
			const Vector<3>& startPoint,
			const Vector<3>& direction,
			double eDiffMax,
			double lenMax
	);

	static double chargeMin;
	static double chargeMax;

	Node* m_connectTo = nullptr;
};

class ElectrostaticLinkPayload : public LinkPayloadBase
{
public:
	ElectrostaticLinkPayload(PhysicalPayloadsRegister* reg, Link* link);

	SOTM_INLINE ElectrostaticPhysicalContext* context() { return static_cast<ElectrostaticPhysicalContext*>(link->physicalContext()); }

	void calculateSecondaryValues(double time) override;
	void calculateRHS(double time) override;
	void addRHSToDelta(double m) override;
	void makeSubIteration(double dt) override;
	void step() override;

	void doBifurcation(double time, double dt) override;

	void init() override;
	std::string getFollowerText() override;


	void setTemperature(double temp);
	double getTemperature();
	double getCurrent();
	double getVoltage();

	double heatCapacity();

	// Parameters
	double radius = 0.01; // 1cm
	//constexpr static Cp = 1.0

	// Primary
	Variable conductivity; // J

	// Secondary
	//double current = 0;
};

SOTM_QUICK_NPF(ElectrostaticNodePayload, ElectrostaticNodePayloadFactory);
SOTM_QUICK_LPF(ElectrostaticLinkPayload, ElectrostaticLinkPayloadFactory);

}


#endif /* LIBSOTM_SOTM_PAYLOADS_ELECTROSTATICS_ELECTROSTATICS_SIMPLE_HPP_ */
