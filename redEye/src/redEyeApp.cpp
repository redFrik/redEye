//--redFrik 2013

//keys:
// i - info on/off
// esc - fullscreen on/off
// f - load fragment shader
// v - load vertex shader
// m - switch drawing mode

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Utilities.h"
#include "cinder/Filesystem.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/audio/FftProcessor.h"  //osx only
#include "cinder/audio/Input.h"
#include "cinder/params/Params.h"
#include "OscListener.h"

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
    void drawWaveform(bool fill);
    void drawSpectrum(bool fill);
    
    gl::Texture             mTextureSnd;
    gl::Texture             mTextureFft;
    gl::GlslProgRef         mShader;
    std::time_t             mTimeFrag;
    std::time_t             mTimeVert;
    fs::path                mPathFrag;
    fs::path                mPathVert;
    std::string             mNameFrag;
    std::string             mNameVert;
    bool                    mHide;          //show/hide error and fps
    std::string             mError;
    std::string             mOsc;
    int                     mMode;          //which shape
    float                   mFps;
    
    audio::Input            mInput;
	std::shared_ptr<float>  mFftLeft;
    //std::shared_ptr<float>  mFftRight;
	audio::PcmBuffer32fRef  mPcmBuffer;
    audio::Buffer32fRef     mBufferLeft;
    //audio::Buffer32fRef     mBufferRight;
    int                     mBufferSize;
    float                   mAmplitude;     //amptracker
    
	params::InterfaceGlRef  mParams;
    osc::Listener           mListener;      //control via parameters via osc
    
    int                     mNumSamples;    //number of samples in waveform
    int                     mDownSample;    //number of samples to skip over
    float                   mWidth;         //line width
    ColorA                  mColorBack;     //background color
    
    //bool                    mVisible0, mVisible1;       //active
    Vec3f                   mScale0, mScale1;           //scales
    Quatf                   mRotate0, mRotate1;         //rotations
    Vec3f                   mTranslate0, mTranslate1;   //positions
    ColorA					mColor0, mColor1;           //waveform colors
};

void redEyeApp::setup() {
    
    //-settings
    mListener.setup(43334);  //osc input port
    
    //--defaults
    mHide= false;       //also keydown 'i'
    mError= "";
    mOsc= "";
    mMode= 5;
    mAmplitude= 0.0f;
    mNumSamples= 512;
    mDownSample= 0;
    mWidth= 1.0f;
    mColorBack= ColorA(0.0f, 0.0f, 0.0f, 1.0f);
    
    //mVisible0= mVisible1= true;
    mScale0= mScale1= Vec3f(1.0f, 1.0f, 1.0f);
    mTranslate0= mTranslate1= Vec3f::zero();
    mColor0= ColorA(1.0f, 0.0f, 0.0f, 1.0f);
    mColor1= ColorA(0.0f, 1.0f, 0.0f, 1.0f);
    
    //--audio
    mInput= audio::Input();     //use default input device
    mInput.start();             //start capturing
    
    //--shaders
    mPathFrag= getPathDirectory(app::getAppPath().string())+"data/_default_frag.glsl";
    mPathVert= getPathDirectory(app::getAppPath().string())+"data/_default_vert.glsl";
    //loadShader();
    
    //--parameter window
    mParams= params::InterfaceGl::create(getWindow(), "redEye", toPixels(Vec2i(200, 400)));
    //mParams->addText("text", ("label=`redEye`"));
    mParams->addParam("NumSamples", &mNumSamples, "min=1 max=2048 step=1");
    mParams->addParam("DownSample", &mDownSample, "min=0 max=2047 step=1");
    mParams->addParam("Width", &mWidth, "min=0 max=1000 step=1");
    mParams->addParam("Back color", &mColorBack, "");
    mParams->addSeparator();
    //mParams->addParam("Visible", &mVisible0);
    mParams->addParam("Scale", &mScale0);
    mParams->addParam("Rotate", &mRotate0);
    mParams->addParam("Translate", &mTranslate0);
    mParams->addParam("Color", &mColor0, "");
    /*mParams->addSeparator();
    mParams->addParam("Visible1", &mVisible1);
    mParams->addParam("Scale1", &mScale1);
    mParams->addParam("Rotate1", &mRotate1);
    mParams->addParam("Translate1", &mTranslate1);
    mParams->addParam("Color1", &mColor1, "");*/
    
    //mParams->addParam("Cube Size", &mObjSize, "min=0.1 max=20.5 step=0.5 keyIncr=z keyDecr=Z");
    mParams->addSeparator();
    mParams->addParam("String00", &mNameFrag, "label=`frag (f):`");
    mParams->addParam("String01", &mNameVert, "label=`vert (v):`");
    mParams->addParam("String02", &mError, "label=`error:`");
    mParams->addParam("String03", &mOsc, "label=`osc:`");
    mParams->addParam("String04", &mMode, "label=`mode (m):`");
    mParams->addParam("String05", &mFps, "label=`fps:`");
}

void redEyeApp::keyDown(ci::app::KeyEvent event) {
	if(event.getChar()=='i') {
        mHide= !mHide;
    } else if(event.getCode()==KeyEvent::KEY_ESCAPE) {
        setFullScreen(!isFullScreen());
    } else if(event.getChar()=='f') {
        fs::path path= getOpenFilePath(mPathFrag);
        if(!path.empty()) {
			mPathFrag= path;
            loadShader();
		}
    } else if(event.getChar()=='v') {
        fs::path path= getOpenFilePath(mPathVert);
		if(!path.empty()) {
			mPathVert= path;
            loadShader();
		}
    } else if(event.getChar()=='m') {
        mMode= (mMode+1)%9;
    }
}
void redEyeApp::loadShader() {
    mError= "";
    try {
        mTimeFrag= fs::last_write_time(mPathFrag);
        mTimeVert= fs::last_write_time(mPathVert);
        mNameFrag= mPathFrag.filename().string();
        mNameVert= mPathVert.filename().string();
        mShader= gl::GlslProg::create(loadFile(mPathVert), loadFile(mPathFrag), NULL);
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
        //std::cout<<"mBufferSize: "<<mBufferSize<<std::endl;
        mBufferLeft= mPcmBuffer->getChannelData(audio::CHANNEL_FRONT_LEFT);
        //mBufferRight= mPcmBuffer->getChannelData(audio::CHANNEL_FRONT_RIGHT);
        mFftLeft= audio::calculateFft(mPcmBuffer->getChannelData(audio::CHANNEL_FRONT_LEFT), mBufferSize/2);
        //mFftRight= audio::calculateFft(mPcmBuffer->getChannelData(audio::CHANNEL_FRONT_LEFT), mBufferSize/2);
        mAmplitude= 0.0f;
        for(uint32_t i= 0; i<mBufferSize; i++) {
            mAmplitude += abs(mBufferLeft->mData[i]);
        }
        mAmplitude /= float(mBufferSize);   //average amplitude
        
        Surface32f mSurfaceSnd(mBufferSize, 1, true);
        Surface32f::Iter sndIter(mSurfaceSnd.getIter());
        uint32_t i= 0;
        while(sndIter.line()) {
            while(sndIter.pixel()) {
                sndIter.r()= mBufferLeft->mData[i];
                i++;
            }
        }
        mTextureSnd= gl::Texture(mSurfaceSnd);
        
        Surface32f mSurfaceFft(mBufferSize/2, 1, true);
        Surface32f::Iter fftIter(mSurfaceFft.getIter());
        uint32_t j= 0;
        float *fftBuffer= mFftLeft.get();
        while(fftIter.line()) {
            while(fftIter.pixel()) {
                fftIter.r()= fftBuffer[j];
                j++;
            }
        }
        mTextureFft= gl::Texture(mSurfaceFft);
    }
    
    //--shaders
    if(mShader!=NULL) {
        if((fs::last_write_time(mPathFrag)>mTimeFrag) || (fs::last_write_time(mPathVert)>mTimeVert)) {
            loadShader();   //hot-loading shader
        }
    }
    
    mFps= getAverageFps();
}

void redEyeApp::drawWaveform(bool fill) {
    if(!mPcmBuffer) {
        return;
    }
    int num= min(mNumSamples, mBufferSize);
    float w= getWindowWidth();
    float a= getWindowHeight()*0.25f;    //wave amplitude
    PolyLine<Vec2f>	line0;
    if(num>=2) {
        for(uint32_t i= 0; i<num; i++) {
            float x= (i/(float)(num-1))*w-(w*0.5f);
            float y= mBufferLeft->mData[i]*a;
            line0.push_back(Vec2f(x, y));
            if(fill) {
                line0.push_back(Vec2f(x, 0.0f-y));
            }
        }
    } else {    //only one sample
        line0.push_back(Vec2f(w* -0.5f, mBufferLeft->mData[0]*a));
        line0.push_back(Vec2f(w*0.5f, mBufferLeft->mData[0]*a));
    }
    gl::draw(line0);
}
void redEyeApp::drawSpectrum(bool fill) {
    if(!mFftLeft) {
        return;
    }
    float *fftBuffer= mFftLeft.get();
    float w= getWindowWidth();
    float a= getWindowHeight()*0.01f;    //spectrum scale
    uint32_t fftSize= mBufferSize/2;
    PolyLine<Vec2f>	line;
    for(uint32_t i= 0; i<fftSize; i++) {
        float x= (i/(float)(fftSize-1))*w-(w*0.5f);
        float y= fftBuffer[i]*a;
        line.push_back(Vec2f(x, 0.0f-y));
        if(fill) {
            line.push_back(Vec2f(x, y));
        }
    }
    gl::draw(line);
}

void redEyeApp::draw() {
	
    gl::clear(mColorBack);
    mTextureSnd.bind(0);
    mTextureFft.bind(1);
    if(mShader!=NULL) {
        mShader->bind();
        mShader->uniform("iResolution", (Vec2f)getWindowSize());
        mShader->uniform("iGlobalTime", (float)getElapsedSeconds());
        mShader->uniform("iAmplitude", mAmplitude);
        mShader->uniform("iChannel0", 0);   //sound
        mShader->uniform("iChannel1", 1);   //fft
    }
    //TODO more drawing modes (lines, bars, rects, cubes...)
    //TODO mDownSample
    
    glPushMatrix();
    gl::translate(getWindowCenter());
    gl::scale(mScale0);
    gl::rotate(mRotate0);
    gl::translate(mTranslate0);
    gl::color(mColor0);
    gl::lineWidth(mWidth);
    switch(mMode) {
        case 0:
            gl::drawSolidRect(Rectf(getWindowBounds()-(getWindowSize()*0.5)));
            break;
        case 1:
            gl::drawSolidRect(Rectf(getWindowBounds()-(getWindowSize()*0.5)).scaledCentered(0.8f));
            break;
        case 2:
            gl::drawSolidTriangle(
                                  (Vec2f)getWindowSize()*Vec2f(0.0f, -0.375f),
                                  (Vec2f)getWindowSize()*Vec2f(-0.375f, 0.375f),
                                  (Vec2f)getWindowSize()*Vec2f(0.375f, 0.375f));
            break;
        case 3:
            gl::drawSolidCircle(Vec2f::zero(), getWindowHeight()*0.4f, 100);
            break;
        case 4:
            gl::drawSphere(Vec3f::zero(), getWindowHeight()*0.4f, 12);   //sphere detail
            break;
        case 5:
            drawWaveform(false);
            break;
        case 6:
            drawWaveform(true);
            break;
        case 7:
            drawSpectrum(false);
            break;
        case 8:
            drawSpectrum(true);
            break;
    }
    glPopMatrix();
    if(mShader!=NULL) {
        mShader->unbind();
        mTextureSnd.unbind();
        mTextureFft.unbind();
    }
    
    if(!mHide) {
        mParams->draw();
    }
}

CINDER_APP_NATIVE(redEyeApp, RendererGl)
