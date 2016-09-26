#include "sotm-gui-internal/frame-maker.hpp"

using namespace sotm;

FrameMaker::FrameMaker(ModelContext *modelContext) :
	m_wireframeDrawer(modelContext)
{
}

void FrameMaker::addActors(vtkRenderer *renderer)
{
	renderer->RemoveAllViewProps();
	renderer->AddActor(m_wireframeDrawer.getActor());
}