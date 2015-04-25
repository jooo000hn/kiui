//  Copyright (c) 2015 Hugo Amiard hugo.amiard@laposte.net
//  This software is provided 'as-is' under the zlib License, see the LICENSE.txt file.
//  This notice and the license may not be removed or altered from any source distribution.

#include <Ui/mkUiConfig.h>
#include <Ui/Nano/mkNanoWindow.h>

#include <Ui/Frame/mkFrame.h>
#include <Ui/Frame/mkStripe.h>
#include <Ui/Nano/mkNanoInk.h>

#include <Ui/Widget/mkWidget.h>

#include <Object/Util/mkMake.h>

//#define NANOVG_GL_USE_UNIFORMBUFFER 1
#ifdef NANOVG_GLEW
#include <Ui/Nano/nanovg/glew.h>
#elif KIUI_EMSCRIPTEN
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include <Ui/Nano/nanovg/nanovg.h>

#if KIUI_EMSCRIPTEN
#define NANOVG_GLES2_IMPLEMENTATION
#else
#define NANOVG_GL2_IMPLEMENTATION
#endif

#include <Ui/Nano/nanovg/nanovg_gl.h>

namespace mk
{
	NanoLayer::NanoLayer(Frame& frame, NanoTarget& target, size_t index)
		: InkLayer(target, index)
		, mTarget(target)
		, mVisible(true)
		, mFrame(frame)
	{}

	NanoLayer::~NanoLayer()
	{}

	unique_ptr<Inkbox> NanoLayer::createInkbox(Frame& frame)
	{
		return make_unique<NanoInk>(frame, *this);
	}

	void NanoLayer::show()
	{
		mVisible = true;
		mFrame.inkbox().show();
		//this->moveToTop();
	}

	void NanoLayer::hide()
	{
		mVisible = false;
		mFrame.inkbox().hide();
	}

	void NanoLayer::moveToTop()
	{
		mTarget.moveToTop(*this);
	}

	void NanoLayer::nanodraw()
	{
		this->draw(mFrame);
	}

	void NanoLayer::draw(Frame& frame)
	{
		static_cast<NanoInk&>(frame.inkbox()).nanodraw();

		if(frame.frameType() >= STRIPE)
			for(Frame* subframe : frame.as<Stripe>().contents())
				this->draw(*subframe);
	}

	NanoTarget::NanoTarget(NanoWindow& window)
		: mWindow(window)
	{}

	void NanoTarget::nanodraw()
	{
		for(auto& layers : mLayers)
			for(InkLayer* layer : layers)
				if(layer->as<NanoLayer>().visible())
					layer->as<NanoLayer>().nanodraw();
	}

	unique_ptr<InkLayer> NanoTarget::createLayer(Frame& frame, size_t z)
	{
		return make_unique<NanoLayer>(frame, *this, z);
	}

	std::map<string, int> NanoWindow::sImages;

	NanoWindow::NanoWindow(size_t width, size_t height, float pixelRatio, string resourcePath)
		: mWidth(width)
		, mHeight(height)
		, mPixelRatio(pixelRatio)
		, mResourcePath(resourcePath)
		, mCtx(nullptr)
	{
#if NANOVG_GL2
		mCtx = nvgCreateGL2(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
#elif NANOVG_GLES2
		mCtx = nvgCreateGLES2(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
#endif
		string fontPath = resourcePath + "interface/fonts/DejaVuSans.ttf";
		nvgCreateFont(mCtx, "dejavu", fontPath.c_str());
		nvgFontSize(mCtx, 14.0f);
		nvgFontFace(mCtx, "dejavu");

		if(mCtx == nullptr)
		{
			printf("Could not init nanovg.\n");
			return;
		}

		mScreenTarget = make_unique<NanoTarget>(*this);
	}

	NanoWindow::~NanoWindow()
	{
		for(auto& kv : sImages)
			nvgDeleteImage(mCtx, kv.second);

#if NANOVG_GL2
		nvgDeleteGL2(mCtx);
#elif NANOVG_GLES2
		nvgDeleteGLES2(mCtx);
#endif
	}

	void NanoWindow::nextFrame()
	{
		nvgBeginFrame(mCtx, mWidth, mHeight, mPixelRatio);

		mScreenTarget->nanodraw();

		nvgEndFrame(mCtx);
	}

	InkTarget& NanoWindow::screenTarget()
	{
		return *mScreenTarget.get();
	}
}