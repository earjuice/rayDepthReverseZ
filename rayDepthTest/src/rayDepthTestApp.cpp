#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/GeomIo.h"
#include "cinder/CameraUi.h"
#include "cinder/Utilities.h"
#include "cinder/CinderImGui.h"
#include "Watchdog.h"
#include "LiveCode.h"

#define REVERSE_Z 

#define FBO_DIM ci::vec2(1920.f, 1080.f)

using namespace ci;
using namespace ci::app;
using namespace std;

void print(std::string msg) {
	ci::app::console() << msg << std::endl;
}

class anaglyphTestApp : public App {
public:
	void setup() override;
	void update() override;
	void draw() override;
	void mouseDown(MouseEvent event) override;
	void mouseUp(MouseEvent event) override;
	void mouseDrag(MouseEvent event) override;
	void mouseWheel(MouseEvent event) override;

	void createGlslformat(fs::path vertex, /* fs::path geometry, */ fs::path fragment, gl::GlslProg::Format *form);
	void setupGlsl(fs::path vertex, /* fs::path geometry, */ fs::path fragment, gl::GlslProg::Format form, gl::GlslProgRef *glslprog, bool aorb);
	void createGlslprog(gl::GlslProgRef &glslprog, gl::GlslProg::Format &glslformat);
	void createBatch(bool aorb);
	void createFbo();

	void createCamera();
	void extractCamUniforms(ci::CameraPersp cam);
	void setCamUniforms(ci::gl::GlslProgRef mGlsl, ci::CameraPersp cam);

	void drawCamUi();
	gl::GlslProg::Format mForm, mFormToy;
	ci::gl::GlslProgRef mGlslProg, mGlslProgToy;
	ci::gl::BatchRef mBatch, mBatchToy;
	ci::gl::VboMeshRef mMesh, mMeshToy;
	ci::gl::FboRef mFbo;

	ci::gl::Texture2dRef mTexCol, mTexDepth;

	ci::CameraPersp mCam;
	ci::CameraUi mCamUi;


	glm::mat4 camMats = mat4(1.);
	glm::mat3 orient = glm::mat3();
	ci::vec3 camEye = vec3();
	float mleft, mright, mtop, mbottom, mnear, mfar, viewDistance;
	ci::vec2 projectionParams;

	fs::path mVertexPath;//fs::path mGeometryPath;
	fs::path mFragmentPath;

	fs::path mVertexPathToy;//fs::path mGeometryPathToy;
	fs::path mFragmentPathToy;
};

void anaglyphTestApp::setup()
{
	ImGui::Initialize(ImGui::Options().window(getWindow()));

	print("make paths");
	mVertexPath = ci::app::getAssetPath("wire_v.glsl");
	//mGeometryPath = ci::app::getAssetPath("wire_g.glsl");
	mFragmentPath = ci::app::getAssetPath("wire_f.glsl");

	mVertexPathToy = ci::app::getAssetPath("Toy_v.glsl");
	//mGeometryPathToy = ci::app::getAssetPath("Toy_g.glsl");
	mFragmentPathToy = ci::app::getAssetPath("Toy_f.glsl");

	print("create Glslformats");
	createGlslformat(mVertexPath,/* mGeometryPath,*/ mFragmentPath, &mForm);
	mForm.define("LINES");

	print("createGlslformat");
	createGlslformat(mVertexPathToy, /*mGeometryPathToy, */ mFragmentPathToy, &mFormToy);
#ifdef REVERSE_Z
	mFormToy.define("REVERSE_Z");
#endif //REVERSE_Z
	mFormToy.define("TRIANGLES");

	print("setupGlsl");
	setupGlsl(mVertexPath,/* mGeometryPath,*/ mFragmentPath, mForm, &mGlslProg, 1);
	setupGlsl(mVertexPathToy, /*mGeometryPathToy,*/ mFragmentPathToy, mFormToy, &mGlslProgToy, 0);

	print("createGlslprog");
	createGlslprog(mGlslProg, mForm);
	createGlslprog(mGlslProgToy, mFormToy);

	print("createFbo");
	createFbo();

	print("createCamera");
	createCamera();

	print("createBatch");
	createBatch(1);
	createBatch(0);


	gl::lineWidth(2.5f);
	glDisable(GL_SCISSOR_TEST);

	print("setup finished");
	//gl::pushViewport(getWindowSize());
}

void anaglyphTestApp::mouseDown(MouseEvent event) {
	mCamUi.mouseDown(event);
}

void anaglyphTestApp::mouseUp(MouseEvent event) {
	mCamUi.mouseUp(event);
}

void anaglyphTestApp::mouseDrag(MouseEvent event) {
	mCamUi.mouseDrag(event);
}

void anaglyphTestApp::mouseWheel(MouseEvent event) {
	mCamUi.mouseWheel(event);
}

void anaglyphTestApp::update()
{
	gl::ScopedMatrices scmat;
	gl::ScopedViewport scvp(mFbo->getSize());
	gl::ScopedFramebuffer scfbo(mFbo);
#ifdef REVERSE_Z
	//gl::ScopedDepth scdep(true);
	gl::ScopedDepth scdep(true, GL_GREATER);
	gl::clearDepth(0.);
	//gl::ScopedFaceCulling scfc(true, GL_BACK);
#else
	gl::ScopedDepth scdep(true);
	//gl::ScopedFaceCulling scfc(true, GL_FRONT);
#endif //REVERSE_Z
	gl::clear();
	extractCamUniforms(mCam);
	setCamUniforms(mGlslProg, mCam);
	setCamUniforms(mGlslProgToy, mCam);

	float fboratio = mFbo->getSize().x / mFbo->getSize().y;

	mGlslProgToy->uniform("iResolution", vec3(mFbo->getSize().x, mFbo->getSize().y, 0.0));
	mGlslProgToy->uniform("iAspect", mCam.getAspectRatio());
	mGlslProgToy->uniform("iGlobalTime", (float)getElapsedSeconds());

	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	{
		gl::ScopedMatrices scmatA;
		gl::setMatricesWindow(mFbo->getSize());
		mBatchToy->draw();
	}

	{
		gl::ScopedMatrices scmatB;
		gl::setMatrices(mCam);
		mBatch->draw();
	}



	auto fboCol = static_pointer_cast<gl::Texture2d>(mFbo->getTextureBase(GL_COLOR_ATTACHMENT0));

	drawCamUi();
}

void anaglyphTestApp::draw() {

	gl::ScopedMatrices scm;
	gl::clear(Color(0, 0, 0));
	gl::color(ColorA::white());

	auto wsz = getWindow()->getSize();
	auto colTex = mFbo->getColorTexture();

	ci::gl::draw(colTex, ci::Rectf(0., 0., wsz.x, (wsz.x * (float)colTex->getHeight()) / (float)colTex->getWidth()));

	gl::scale(vec3(wsz.x / ((float)colTex->getWidth())));
	gl::drawStrokedRect(colTex->getBounds());

}

void anaglyphTestApp::drawCamUi() {
	ImGui::ScopedWindow scwi("Camera");
	static float fov = mCam.getFov();
	if (ImGui::DragFloat("FOV", &fov)) {
		mCam.setFov(fov);
	}
}

void anaglyphTestApp::setupGlsl(fs::path vertex, /*fs::path geometry,*/ fs::path fragment, gl::GlslProg::Format form, gl::GlslProgRef *glslprog, bool aorb) {

	wd::unwatch(vertex);
	//wd::unwatch(geometry);
	wd::unwatch(fragment);

	auto cb = [this, vertex, /*geometry,*/ fragment, form, glslprog, aorb](const fs::path &path) {

		print("auto cb");
		reza::live::glsl(
			vertex,
			fragment,
			//geometry,
			form,
			[this]() {

		},
			[this, glslprog, aorb](gl::GlslProgRef result, std::vector<std::string> sources) {
			print("makeCurrentContext");
			getWindow()->getRenderer()->makeCurrentContext(true);
			*glslprog = result;
			createBatch(aorb);

			print("mGlslInitialized");
		},
			[this](ci::Exception exc)
		{
			print(exc.what());
		});

	};

	wd::watch(vertex, cb);
	//wd::watch(geometry, cb);
	wd::watch(fragment, cb);

}

void anaglyphTestApp::createGlslformat(fs::path vertex, /*fs::path geometry,*/ fs::path fragment, gl::GlslProg::Format *form) {

	*form = gl::GlslProg::Format()
		.vertex(loadAsset(vertex))
		//.geometry(loadAsset(geometry))
		.fragment(loadAsset(fragment));

}

void anaglyphTestApp::createGlslprog(gl::GlslProgRef &glslprog, gl::GlslProg::Format &glslformat) {

	try {
		//// Load our program.
		glslprog = gl::GlslProg::create(glslformat);
	}
	catch (gl::GlslProgExc &exc) {
		app::console() << "GlslProgExc error: " << std::endl;
		app::console() << exc.what();
	}
	catch (gl::GlslProgCompileExc &exc) {
		app::console() << "GlslProgCompileExc error: " << std::endl;
		app::console() << exc.what();
	}
	catch (gl::GlslProgLinkExc &exc) {
		app::console() << "GlslProgLinkExc error: " << std::endl;
		app::console() << exc.what();
	}
	catch (gl::GlslNullProgramExc &exc) {
		app::console() << "GlslNullProgramExc error: " << std::endl;
		app::console() << exc.what();
	}
	catch (Exception &exc) {
		ci::app::console() << "exception: " << &exc << endl;
	}

}

void anaglyphTestApp::createBatch(bool aorb)
{
	if (aorb) {
		console() << "create mBatch" << endl;
		mMesh = ci::gl::VboMesh::create(geom::WireCube().size(10., 10., 10.).subdivisions(2));
		mBatch = ci::gl::Batch::create(mMesh, mGlslProg);
	}
	else {
		console() << "create mBatchToy" << endl;
		vec2 fbohlfsz = (vec2)mFbo->getSize()*vec2(.5);
		mMeshToy = ci::gl::VboMesh::create(geom::Plane().size(mFbo->getSize()).axes(vec3(1., 0., 0.), vec3(0., 1., 0.)).origin(vec3(fbohlfsz.x, fbohlfsz.y, 0.)));
		mBatchToy = ci::gl::Batch::create(mMeshToy, mGlslProgToy);
	}

}



void anaglyphTestApp::createFbo()
{

	mTexCol = ci::gl::Texture2d::create(FBO_DIM.x, FBO_DIM.y, ci::gl::Texture2d::Format().target(GL_TEXTURE_2D).internalFormat(GL_RGBA));
	mTexDepth = ci::gl::Texture2d::create(FBO_DIM.x, FBO_DIM.y, ci::gl::Texture2d::Format().target(GL_TEXTURE_2D).internalFormat(GL_DEPTH_COMPONENT32F));

	ci::gl::Fbo::Format fboFormat;
	fboFormat.label("Render_Fbo");
	fboFormat.attachment(GL_COLOR_ATTACHMENT0, mTexCol);
	fboFormat.attachment(GL_DEPTH_ATTACHMENT, mTexDepth);
	fboFormat.depthBuffer(GL_DEPTH_COMPONENT32F);
	fboFormat.samples(8);

	mFbo = ci::gl::Fbo::create(FBO_DIM.x, FBO_DIM.y, fboFormat);

}

void anaglyphTestApp::createCamera() {

	mCam.setPerspective(60., 2, .1, 500.f);
	mCam.setWorldUp(ci::vec3(0.0f, 1.0f, 0.0f));
	mCam.lookAt(ci::vec3(0.0f, 0.0f, 10.f), ci::vec3(0.0f, 0.0f, 0.0f));
	mCam.setPivotDistance(glm::distance(ci::vec3(0.0f, 0.0f, 10.f), ci::vec3(0.0f, 0.0f, 0.0f)));
	mCam.setAspectRatio(FBO_DIM.x / FBO_DIM.y);
#ifdef REVERSE_Z
	ci::gl::enableDepthReversed();
	//mCam.setInfinitePerspective(60., 2, .1);
	mCam.setInfiniteFarClip(true);
	gl::clipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
#endif//REVERSE_Z

	mCamUi.setCamera(&mCam);
}

#ifdef REVERSE_Z
glm::mat4 MakeInfReversedZProjRH(float fovY_radians, float aspectWbyH, float zNear)
{
	float f = 1.0f / tan(fovY_radians / 2.0f);
	return glm::mat4(
		f / aspectWbyH, 0.0f, 0.0f, 0.0f,
		0.0f, f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, zNear, 0.0f);
}
#endif//REVERSE_Z


void anaglyphTestApp::extractCamUniforms(ci::CameraPersp cam) {
	mCam = cam;
	mCam.lookAt(cam.getEyePoint(), cam.getPivotPoint());
	camEye = mCam.getEyePoint();

#ifdef REVERSE_Z
	glm::mat4 rMat = MakeInfReversedZProjRH(ci::toRadians(mCam.getFov()), mCam.getAspectRatio(), mCam.getNearClip());
	camMats = rMat * mCam.getViewMatrix();
#else
	camMats = mCam.getProjectionMatrix() * mCam.getViewMatrix();
#endif//REVERSE_Z
	orient = glm::mat3(mCam.getInverseViewMatrix());

	cam.getFrustum(&mleft, &mtop, &mright, &mbottom, &mnear, &mfar);
	viewDistance = cam.getAspectRatio() / math< float >::abs(mright - mleft) * mnear;
	projectionParams = ci::vec2(mfar / (mfar - mnear), (-mfar * mnear) / (mfar - mnear));

}

void  anaglyphTestApp::setCamUniforms(ci::gl::GlslProgRef mGlsl, ci::CameraPersp cam) {

	mGlsl->uniform("uCameraMatsViewProj", camMats);
	mGlsl->uniform("iCamEyePos", camEye);
	mGlsl->uniform("uCameraDirection", orient);
	mGlsl->uniform("iCamViewDir", cam.getViewDirection());

	mGlsl->uniform("uViewDistance", viewDistance);//shFrag
	mGlsl->uniform("uProjectionParams", projectionParams);//depthFunc
	mGlsl->uniform("zNear", mnear);//depthFunc
	mGlsl->uniform("zFar", mfar);//depthFunc
}

auto settingsFunc = [](App::Settings *settings)
{
	//settings->setConsoleWindowEnabled();
	//settings->setHighDensityDisplayEnabled();
	//settings->prepareWindow(Window::Format().size(vec2(960, 960)).title("malfunkn 360"));
	settings->setFrameRate(30.0f);

	//settings->setQuitOnLastWindowCloseEnabled(false);
	//console() << "isQuitOnLastWindowCloseEnabled " << settings->isQuitOnLastWindowCloseEnabled() << endl;
	//settings->disableFrameRate();
	//#if defined( CINDER_MSW )
	//#endif
};

CINDER_APP(anaglyphTestApp, RendererGl(RendererGl::Options().version(4, 6).coreProfile().msaa(8).depthBufferDepth(32)), settingsFunc)

