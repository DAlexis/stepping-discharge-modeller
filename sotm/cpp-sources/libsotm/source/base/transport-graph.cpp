#include "sotm/base/transport-graph.hpp"
#include "sotm/base/model-context.hpp"
#include "sotm/base/physical-payload.hpp"
#include "sotm/utils/utils.hpp"

using namespace sotm;

void GraphRegister::addLink(Link* link)
{
	ASSERT(m_links.find(link) == m_links.end() && m_linksToAdd.find(link) == m_linksToAdd.end(),
			"Link already added to graph register");
	if (m_iteratingNow)
	{
		m_linksToAdd.insert(link);
	} else {
		m_links.insert(link);
	}
}

void GraphRegister::addNode(Node* node)
{
	ASSERT(m_nodes.find(node) == m_nodes.end() && m_nodesToAdd.find(node) == m_nodesToAdd.end(),
			"Link already added to graph register");
	if (m_iteratingNow)
	{
		m_nodesToAdd.insert(node);
	} else {
		m_nodes.insert(node);
	}
}

void GraphRegister::rmLink(Link* link)
{
	ASSERT(m_links.find(link) != m_links.end(), "Link does not exists in graph register");
	if (m_iteratingNow)
	{
		m_linksToDelete.insert(link);
	} else {
		m_links.erase(link);
	}
}

void GraphRegister::rmNode(Node* node)
{
	ASSERT(m_nodes.find(node) != m_nodes.end(), "Link does not exists in graph register");
	if (m_iteratingNow)
	{
		m_nodesToDelete.insert(node);
	} else {
		m_nodes.erase(node);
	}
}

void GraphRegister::applyNodeVisitor(NodeVisitor v)
{
	beginIterating();
	RunOnceOnExit end([this](){ endIterating(); });
	for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it)
	{
		v(*it);
	}
}

void GraphRegister::applyLinkVisitor(LinkVisitor v)
{
	beginIterating();
	RunOnceOnExit end([this](){ endIterating(); });
	for (auto it = m_links.begin(); it != m_links.end(); ++it)
	{
		v(*it);
	}
}

void GraphRegister::beginIterating()
{
	m_iteratingNow = true;
}

void GraphRegister::endIterating()
{
	m_iteratingNow = false;
	m_nodes.insert(m_nodesToAdd.begin(), m_nodesToAdd.end());
	m_nodesToAdd.clear();
	m_links.insert(m_linksToAdd.begin(), m_linksToAdd.end());
	m_linksToAdd.clear();
	for (auto it = m_nodesToDelete.begin(); it != m_nodesToDelete.end(); ++it)
	{
		m_nodes.erase(*it);
	}
	m_nodesToDelete.clear();
	for (auto it = m_linksToDelete.begin(); it != m_linksToDelete.end(); ++it)
	{
		m_links.erase(*it);
	}
	m_linksToDelete.clear();
}

////////////////////////////
// Node
Node::Node(ModelContext* context, Point<3> pos) :
	pos(pos),
	m_context(context)
{
	payload.reset(
		m_context->createNodePayload(this)
	);
	m_context->graphRegister.addNode(this);
}

Node::~Node()
{
	m_context->graphRegister.rmNode(this);
}

void Node::addLink(Link* link)
{
	m_links.insert(link);
}

void Node::removeLink(Link* link)
{
	size_t count = m_links.erase(link);
	ASSERT(count != 0, "Removing link that was not connected");
}

Link::Link(ModelContext* context) :
	m_context(context)
{
	/// @todo Should we do this before connecting link to nodes?
	payload.reset(
		m_context->createLinkPayload(this)
	);
	m_context->graphRegister.addLink(this);
}

Link::Link(ModelContext* context, Node* nodeFrom, Point<3> pointTo) :
	m_context(context)
{
	ASSERT(nodeFrom != nullptr, "Cannot create link with nullptr node");
	m_context->graphRegister.addLink(this);
	PtrWrap<Node> newNode = PtrWrap<Node>::make(context, pointTo);
	payload.reset(
		m_context->createLinkPayload(this)
	);
	connect(nodeFrom, newNode);
}

Link::~Link()
{
	m_context->graphRegister.rmLink(this);
}

void Link::connect(Node* n1, Node* n2)
{
	m_n1.assign(n1);
	m_n2.assign(n2);
}


