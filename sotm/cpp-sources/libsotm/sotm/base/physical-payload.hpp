#ifndef PHYSICAL_CONTEXT_HPP_INCLUDE
#define PHYSICAL_CONTEXT_HPP_INCLUDE

#include "sotm/base/transport-graph.hpp"
#include "sotm/base/time-iter.hpp"
#include "sotm/utils/memory.hpp"
#include "sotm/base/parallel.hpp"

#include <vector>
#include <set>
#include <string>

namespace sotm
{

class AnyPhysicalPayloadBase;
class ModelContext;

class PhysicalPayloadsRegister : public IContinuousTimeIterable
{
public:
	PhysicalPayloadsRegister(const ParallelSettings* parallelSettings = &ParallelSettings::parallelDisabled);
	void add(AnyPhysicalPayloadBase* payload);
	void remove(AnyPhysicalPayloadBase* payload);

	void clearSubiteration() override final;
	void calculateSecondaryValues(double time) override final;
	void calculateRHS(double time) override final;
	void addRHSToDelta(double m) override final;
	void makeSubIteration(double dt) override final;
	void step() override final;
	double getMinimalStepsCount() override final;

    void destroyAll();

private:
	void rebuildPayloadsVectorIfNeeded();
	std::set<AnyPhysicalPayloadBase*> m_payloads;
	std::vector<AnyPhysicalPayloadBase*> m_payloadsVector;
	bool m_payloadsVectorDirty = true;

	const ParallelSettings* m_parallelSettings;
};

class AnyPhysicalPayloadBase : public IContinuousTimeIterable, public IBifurcationTimeIterable
{
public:
	AnyPhysicalPayloadBase(PhysicalPayloadsRegister* reg);
	virtual ~AnyPhysicalPayloadBase();

	/**
     * This function will be called when after payload is connected to link or node.
	 * Initial values for variables may be set here
	 *
	 * todo: check real usage of this function
	 */
	virtual void init() { }

	virtual void clearSubiteration() { }

	virtual void calculateSecondaryValues(double time) { }

	/// Get object color for visualization purpose. Color is RGB array. Each color is from interval [0.0; 1.0]
	virtual void getColor(double* rgb);

	/// Get vector of 3 parameters. It may be one vector or 3 scalar things
	virtual void getParametersVector(double* parameters);

	/// Get object size for visualization purpose. Normal size if 1.0
	virtual double getSize();

	virtual std::string getFollowerText();

	virtual void prepareBifurcation(double time, double dt) override { }

    /**
     * @brief onDeletePayload
     * This function should be called by physical payload
     * when physics things that object holding this payload should die.
     * derived classes must call this func BEFORE calling their own version of this func
     */
    virtual void onDeletePayload();

protected:
	constexpr static double defaultColor[3] = {1.0, 0.8, 0.3};

private:
	PhysicalPayloadsRegister *m_payloadsRegister;
};

class IPhysicalContext : public IContinuousTimeIterable, public IBifurcationTimeIterable
{
public:
	virtual ~IPhysicalContext() {}
	virtual void connectModel(ModelContext* m) = 0;
    /**
     * @brief This should be called by model before it will be deleted.
     * Physical context here may release some PtrWrap'pers to some specific
     * nodes or links
     */
    virtual void onDestroy() = 0;
    /**
     * @brief This function will be called after init() of all payloads.
     * It is a place to first prepairing for optimizatiors i.e.
     */
    virtual void init() = 0;
};

class PhysicalContextBase : public IPhysicalContext
{
public:
	void connectModel(ModelContext* m) override { m_model = m; }
	virtual void clearSubiteration() override { }
	virtual void prepareBifurcation(double time, double dt) override { }
    virtual void onDestroy() override {}
    virtual void init() override {}

    ModelContext& model() { return *m_model; }
protected:
	ModelContext* m_model = nullptr;
};

struct BranchingParameters
{
	bool needBranching = false;
	Direction direction;
	double length = 0.0;
};

class NodePayloadBase : public AnyPhysicalPayloadBase
{
public:
	NodePayloadBase(PhysicalPayloadsRegister* reg, Node* node);
	void onDeletePayload() override;

	virtual void getBranchingParameters(double time, double dt, BranchingParameters& branchingParameters);

protected:
	PtrWrap<Node> node;
};

class LinkPayloadBase : public AnyPhysicalPayloadBase
{
public:
	LinkPayloadBase(PhysicalPayloadsRegister* reg, Link* link);
	void onDeletePayload() override;

protected:
	PtrWrap<Link> link;
};

class INodePayloadFactory
{
public:
	virtual ~INodePayloadFactory() {}
	virtual NodePayloadBase* create(PhysicalPayloadsRegister* reg, Node* node) = 0;
};

class ILinkPayloadFactory
{
public:
	virtual ~ILinkPayloadFactory() {}
	virtual LinkPayloadBase* create(PhysicalPayloadsRegister* reg, Link* link) = 0;
};

// Quickly define typical factory for node payload
#define SOTM_QUICK_NPF(Classname, FactoryName) \
	class FactoryName : public INodePayloadFactory \
	{ \
	public: \
		Classname* create(PhysicalPayloadsRegister* reg, Node* node) override final \
		{ return new Classname(reg, node); } \
	};

// Quickly define typical factory for link payload
#define SOTM_QUICK_LPF(Classname, FactoryName) \
	class FactoryName : public ILinkPayloadFactory \
	{ \
	public: \
		Classname* create(PhysicalPayloadsRegister* reg, Link* link) override final \
		{ return new Classname(reg, link); } \
	};

// Quickly add cast() method for physical context. There is no real need in this method, but it simplify casting
#define SOTM_QUICK_PHYS_CONT_CASTS(Classname) \
	static inline Classname* cast(IPhysicalContext* context) \
	{ \
		return static_cast<Classname*>(context); \
	} \
	 \
	static inline const Classname* cast(const IPhysicalContext* context) \
	{ \
		return static_cast<const Classname*>(context); \
	} \

}

#endif // PHYSICAL_CONTEXT_HPP_INCLUDE
