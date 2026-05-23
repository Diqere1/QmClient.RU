#include <engine/client/backend_sdl.h>
#include <engine/client/graphics_threaded.h>

#include <gtest/gtest.h>

TEST(GraphicsRenderTarget, CommandStructsExposeExpectedFields)
{
	CCommandBuffer::SCommand_RenderTarget_Create Create;
	Create.m_TargetId = 7;
	Create.m_Width = 320;
	Create.m_Height = 180;
	EXPECT_EQ(Create.m_Cmd, CCommandBuffer::CMD_RENDER_TARGET_CREATE);
	EXPECT_EQ(Create.m_TargetId, 7);
	EXPECT_EQ(Create.m_Width, 320);
	EXPECT_EQ(Create.m_Height, 180);

	CCommandBuffer::SCommand_RenderTarget_Draw Draw;
	Draw.m_TargetId = 4;
	Draw.m_X = 1.0f;
	Draw.m_Y = 2.0f;
	Draw.m_W = 3.0f;
	Draw.m_H = 4.0f;
	EXPECT_EQ(Draw.m_Cmd, CCommandBuffer::CMD_RENDER_TARGET_DRAW);
	EXPECT_EQ(Draw.m_TargetId, 4);
	EXPECT_FLOAT_EQ(Draw.m_X, 1.0f);
	EXPECT_FLOAT_EQ(Draw.m_Y, 2.0f);
	EXPECT_FLOAT_EQ(Draw.m_W, 3.0f);
	EXPECT_FLOAT_EQ(Draw.m_H, 4.0f);
}

TEST(GraphicsRenderTarget, BackendCapabilitiesDefaultToNoRenderTarget)
{
	SBackendCapabilities Capabilities{};
	EXPECT_FALSE(Capabilities.m_RenderTargets);
}
