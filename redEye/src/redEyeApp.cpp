//--redFrik 2013

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/Utilities.h"
#include "cinder/params/Params.h"
#include "cinder/audio/FftProcessor.h"
#include "cinder/audio/Input.h"
#include "OscListener.h"
#include "cinder/Filesystem.h"
//#include "cinder/gl/Texture.h"
#include "cinder/gl/GlslProg.h"
//#include "Resources.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class redEyeApp : public AppNative {
  public:
	void setup();
    void update();
	void draw();
    void keyDown(KeyEvent event);
    void loadShader();
    
    osc::Listener           mListener;      //control via parameters via osc
    params::InterfaceGlRef  mParams;
    audio::Input            mInput;
	std::shared_ptr<float>  mFftDataRef;
	audio::PcmBuffer32fRef  mPcmBuffer;
    audio::Buffer32fRef     mBufferLeft;
    audio::Buffer32fRef     mBufferRight;
    int                     mBufferSize;
//    gl::TextureRef          mTexture;
	gl::GlslProgRef         mShader;
    std::time_t             mTimeVert;
    std::time_t             mTimeFrag;
    fs::path                mPathVert;
    fs::path                mPathFrag;
//    Surface                 mSurface;
    
    int                     mNumSamples;    //number of samples in waveform
    int                     mDownSample;    //number of samples to skip over
    float                   mAmplitude;     //waveform y axis scale
    float                   mWidth;         //line width
    ColorA                  mColorBack;     //background color
    
    bool                    mVisible0, mVisible1;       //active
    Vec3f                   mScale0, mScale1;           //scales
    Quatf                   mRotate0, mRotate1;         //rotations
    Vec3f                   mTranslate0, mTranslate1;   //positions
    ColorA					mColor0, mColor1;           //waveform colors
    
    bool                    mHide;          //show/hide parameters and fps
    std::string             mError;
    std::string             mOsc;
    
};

void redEyeApp::setup() {
    
    //-settings
    mListener.setup(43334);  //osc input port
    
    //--defaults
    mNumSamples= 512;
    mDownSample= 0;
    mAmplitude= 100.0f;
    mWidth= 1.0f;
    mColorBack= ColorA(0.0f, 0.0f, 0.0f, 1.0f);
    
    mVisible0= mVisible1= true;
    mScale0= mScale1= Vec3f(1.0f, 1.0f, 1.0f);
    mTranslate0= mTranslate1= Vec3f::zero();
    mColor0= ColorA(1.0f, 0.0f, 0.0f, 1.0f);
    mColor1= ColorA(0.0f, 1.0f, 0.0f, 1.0f);
    
    mHide= false;   //also keydown 'i'
    mError= "";
    mOsc= "";
    
    //--parameter window
    mParams= params::InterfaceGl::create(getWindow(), "redEye", toPixels(Vec2i(200, 400)));
    //mParams->addText("text", ("label=`redEye`"));
    mParams->addParam("NumSamples", &mNumSamples, "min=1 max=2048 step=1");
    mParams->addParam("DownSample", &mDownSample, "min=0 max=2047 step=1");
    mParams->addParam("mAmplitude", &mAmplitude, "");
    mParams->addParam("mWidth", &mWidth, "min=0 max=1000 step=1");
    mParams->addParam("Back color", &mColorBack, "");
    mParams->addSeparator();
    mParams->addParam("Visible0", &mVisible0);
    mParams->addParam("Scale0", &mScale0);
    mParams->addParam("Rotate0", &mRotate0);
    mParams->addParam("Translate0", &mTranslate0);
    mParams->addParam("Color0", &mColor0, "");
    mParams->addSeparator();
    mParams->addParam("Visible1", &mVisible1);
    mParams->addParam("Scale1", &mScale1);
    mParams->addParam("Rotate1", &mRotate1);
    mParams->addParam("Translate1", &mTranslate1);
    mParams->addParam("Color1", &mColor1, "");
    
    //mParams->addParam("Cube Size", &mObjSize, "min=0.1 max=20.5 step=0.5 keyIncr=z keyDecr=Z");
    mParams->addSeparator();
    mParams->addParam("String", &mError, "label=`Shader error:`");
    mParams->addParam("String2", &mOsc, "label=`Osc input:`");
    
    //--audio
    mInput= audio::Input();     //use default input device
    mInput.start();             //start capturing
    
    //--shaders
//    mTexture= gl::Texture::create( loadImage( loadResource( RES_IMAGE_JPG ) ) );
    mPathVert= getPathDirectory(app::getAppPath().string())+"shaders/default_vert.glsl";
    mPathFrag= getPathDirectory(app::getAppPath().string())+"shaders/default_frag.glsl";
    loadShader();
}

void redEyeApp::keyDown(ci::app::KeyEvent event) {
	if(event.getChar()=='i') {
        mHide= !mHide;
    } else if(event.getCode()==KeyEvent::KEY_ESCAPE) {
        setFullScreen(!isFullScreen());
    } else if(event.getChar()=='v') {
        fs::path path= getOpenFilePath(mPathVert);
		if(!path.empty()) {
			mPathVert= path;
            loadShader();
		}
    } else if(event.getChar()=='f') {
        fs::path path= getOpenFilePath(mPathFrag);
        if(!path.empty()) {
			mPathFrag= path;
            loadShader();
		}
        //std::cout<<getPathDirectory(app::getAppPath().string())+"frag shaders"<<std::endl;
    }
}
void redEyeApp::loadShader() {
    mError= "";
    try {
        mTimeVert= fs::last_write_time(mPathVert);
        mTimeFrag= fs::last_write_time(mPathFrag);
        mShader= gl::GlslProg::create(loadFile(mPathVert), loadFile(mPathFrag));
    }
    catch(gl::GlslProgCompileExc &exc) {
        mError= exc.what();
    }
    catch(...) {
        mError= "Unable to load shader";
    }
}

void redEyeApp::update() {
    
    //--osc input
    while(mListener.hasWaitingMessages()) {
		osc::Message msg;
		mListener.getNextMessage(&msg);
        mOsc= msg.getAddress();
        for(uint32_t i= 0; i<msg.getNumArgs(); i++) {
            mOsc= mOsc+" "+msg.getArgAsString(i, true);
        }
        if(msg.getAddress()=="/numSamples") {
            mNumSamples= math<int32_t>::clamp(msg.getArgAsInt32(0, true), 1, 2048);
        } else if(msg.getAddress()=="/downSample") {
            mDownSample= math<int32_t>::clamp(msg.getArgAsInt32(0, true), 0, 2047);
        } else if(msg.getAddress()=="/amplitude") {
            mAmplitude= msg.getArgAsFloat(0, true);
        } else if(msg.getAddress()=="/width") {
            mWidth= math<float>::clamp(msg.getArgAsFloat(0, true), 0, 1000);
        } else if(msg.getAddress()=="/colorBack") {
            ColorA col= ColorA(0, 0, 0, 1);
            for(uint32_t i= 0; i<min(msg.getNumArgs(), 4); i++) {
                col[i]= msg.getArgAsFloat(i, true);
            }
            mColorBack.set(col.r, col.g, col.b, col.a);
        
        } else if(msg.getAddress()=="/scale0") {
            Vec3f sca= Vec3f(1.0f, 1.0f, 1.0f);
            for(uint32_t i= 0; i<min(msg.getNumArgs(), 3); i++) {
                msg.getArgAsFloat(i, true);
            }
            mScale0.set(sca.x, sca.y, sca.z);
        } else if(msg.getAddress()=="/scale1") {
            Vec3f sca= Vec3f(1.0f, 1.0f, 1.0f);
            for(uint32_t i= 0; i<min(msg.getNumArgs(), 3); i++) {
                msg.getArgAsFloat(i, true);
            }
            mScale1.set(sca.x, sca.y, sca.z);
        } else if(msg.getAddress()=="/rotate0") {
            Vec3f rot= Vec3f::zero();
            for(uint32_t i= 0; i<min(msg.getNumArgs(), 3); i++) {
                rot[i]= msg.getArgAsFloat(i, true);
            }
            mRotate0.set(rot.x, rot.y, rot.z);
        } else if(msg.getAddress()=="/rotate1") {
            Vec3f rot= Vec3f::zero();
            for(uint32_t i= 0; i<min(msg.getNumArgs(), 3); i++) {
                rot[i]= msg.getArgAsFloat(i, true);
            }
            mRotate1.set(rot.x, rot.y, rot.z);
        } else if(msg.getAddress()=="/translate0") {
            Vec3f tra= Vec3f::zero();
            for(uint32_t i= 0; i<min(msg.getNumArgs(), 3); i++) {
                tra[i]= msg.getArgAsFloat(i, true);
            }
            mTranslate0.set(tra.x, tra.y, tra.z);
        } else if(msg.getAddress()=="/translate1") {
            Vec3f tra= Vec3f::zero();
            for(uint32_t i= 0; i<min(msg.getNumArgs(), 3); i++) {
                tra[i]= msg.getArgAsFloat(i, true);
            }
            mTranslate1.set(tra.x, tra.y, tra.z);
        } else if(msg.getAddress()=="/color0") {
            ColorA col= ColorA(0, 0, 0, 1);
            for(uint32_t i= 0; i<min(msg.getNumArgs(), 4); i++) {
                col[i]= msg.getArgAsFloat(i, true);
            }
            mColor0.set(col.r, col.g, col.b, col.a);
        } else if(msg.getAddress()=="/color1") {
            ColorA col= ColorA(0, 0, 0, 1);
            for(uint32_t i= 0; i<min(msg.getNumArgs(), 4); i++) {
                col[i]= msg.getArgAsFloat(i, true);
            }
            mColor1.set(col.r, col.g, col.b, col.a);
        }
	}
    
    //--audio input
    mPcmBuffer= mInput.getPcmBuffer();
    if(mPcmBuffer) {
        mBufferSize= mPcmBuffer->getSampleCount();
        //console()<<"mBufferSize: "<<mBufferSize<<std::endl;
        mBufferLeft= mPcmBuffer->getChannelData(audio::CHANNEL_FRONT_LEFT);
        mBufferRight= mPcmBuffer->getChannelData(audio::CHANNEL_FRONT_RIGHT);
        uint16_t bandCount= 512;
        mFftDataRef= audio::calculateFft(mPcmBuffer->getChannelData(audio::CHANNEL_FRONT_LEFT), bandCount);
        //mFftDataRef= audio::calculateFft(mPcmBuffer->getChannelData(audio::CHANNEL_FRONT_RIGHT), bandCount);
    }
    
    //--shaders
    if((fs::last_write_time(mPathVert)>mTimeVert)||(fs::last_write_time(mPathFrag)>mTimeFrag)) {
        loadShader();
    }
}

void redEyeApp::draw() {
	//gl::enableDepthRead();
	//gl::enableDepthWrite();
	gl::clear(mColorBack);
    
//    mTexture->enableAndBind();
	mShader->bind();
    mShader->uniform("iResolution", (Vec2f)getWindowSize());
    mShader->uniform("iGlobalTime", (float)getElapsedSeconds());
    
	//mShader->uniform("tex0", 0);
	//mShader->uniform("sampleOffset", Vec2f(cos(mAngle), sin(mAngle))*(3.0f/getWindowWidth()));
    
    
    //TODO drawing modes (lines, bars, rects, cubes...)
    //TODO mDownSample
    
    int num= min(mNumSamples, mBufferSize);
    float halfWidth= getWindowWidth()*0.5f;
    gl::lineWidth(mWidth);
    
	gl::pushMatrices();
    gl::translate(getWindowCenter());
    gl::scale(mScale0);
	gl::rotate(mRotate0);
    gl::translate(mTranslate0);
	gl::color(mColor0);
    PolyLine<Vec2f>	line0;
    if(num>=2) {
        for(uint32_t i= 0; i<num; i++) {
            float x= (i/(float)(num-1))*halfWidth*2-halfWidth;
            float y= mBufferLeft->mData[i]*mAmplitude;
            line0.push_back(Vec2f(x, y));
        }
    } else {    //only one sample
        line0.push_back(Vec2f(-halfWidth, mBufferLeft->mData[0]*mAmplitude));
        line0.push_back(Vec2f(halfWidth, mBufferLeft->mData[0]*mAmplitude));
    }
	gl::draw(line0);
    gl::popMatrices();
    
    gl::pushMatrices();
    gl::translate(getWindowCenter());
    gl::scale(mScale1);
	gl::rotate(mRotate1);
    gl::translate(mTranslate1);
	gl::color(mColor1);
    PolyLine<Vec2f>	line1;
    if(num>=2) {
        for(uint32_t i= 0; i<num; i++) {
            float x= (i/(float)(num-1))*halfWidth*2-halfWidth;
            float y= mBufferRight->mData[i]*mAmplitude;
            line1.push_back(Vec2f(x, y));
        }
    } else {    //only one sample
        line1.push_back(Vec2f(-halfWidth, mBufferRight->mData[0]*mAmplitude));
        line1.push_back(Vec2f(halfWidth, mBufferRight->mData[0]*mAmplitude));
    }
	gl::draw(line1);
    gl::popMatrices();
    
//    mTexture->unbind();
    mShader->unbind();
    
    if(!mHide) {
        mParams->draw();
        gl::drawString("fps: "+toString(getAverageFps()), Vec2f(30.0f, getWindowHeight()-20.0f), Color(1, 1, 1), Font("Verdana", 12));
    }
}

CINDER_APP_NATIVE(redEyeApp, RendererGl)
