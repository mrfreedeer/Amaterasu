#include "Engine/Renderer/Renderer.hpp"

Renderer::Renderer(RendererConfig const& config)
{

}

DescriptorHeap* Renderer::CreateDescriptorHeap(DescriptorHeapDesc const& desc)
{

	return new DescriptorHeap(desc);

}

