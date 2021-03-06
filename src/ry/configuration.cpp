#include "configuration.h"

#include <Kin/TM_PairCollision.h>
#include <Kin/frame.h>
#include <Kin/proxy.h>

ry::Configuration::Configuration(){
//  LOG(0) <<"create " <<this;
}

ry::Configuration::~Configuration(){
//  LOG(0) <<"destroy " <<this;
  for(auto& d:cameras) d->kin=NULL;
}

void ry::Configuration::clear(){
  K.set()->clear();
  for(auto& d:cameras) d->update(STRING("clear"));
}

void ry::Configuration::addFile(const std::string& file){
  K.set()->addModel(file.c_str());
  for(auto& d:cameras) d->update(STRING("addFile '" <<file <<"'"));
}

void ry::Configuration::addFrame(const std::string& name, const std::string& parent, const std::string& args){
  K.writeAccess();
  rai::Frame *f = new rai::Frame(K());
  f->name = name;

  if(parent.size()){
    rai::Frame *p = K().getFrameByName(parent.c_str());
    if(p) f->linkFrom(p);
  }

  rai::String(args) >>f->ats;
  f->read(f->ats);

  if(f->parent) f->X = f->parent->X * f->Q;
  K.deAccess();
  for(auto& d:cameras) d->update(STRING("addFrame '" <<name <<'(' <<parent <<")'"));
}

void ry::Configuration::delFrame(const std::string& name){
  K.writeAccess();
  rai::Frame *p = K().getFrameByName(name.c_str(), true);
  if(p) delete p;
  K.deAccess();
  for(auto& d:cameras) d->update(STRING("delFrame '" <<name <<"'"));
}

void ry::Configuration::editorFile(const std::string& filename){
  cout <<"Edit the configuration in the editor. Whenever you save, the display will update. Wrong syntax errors will be displayed here. Hit SPACE in the window to animate, ENTER to force reload after error, q to exit this mode." <<endl;
  K.writeAccess();
  K().clear();
  K().addModel(filename.c_str());
  rai::system(STRING("emacs " <<filename <<" &"));
  {
    OpenGL gl;
    gl.add(glStandardScene);
    gl.add(K());
    editConfiguration(filename.c_str(), K(), gl);
  }
  K.deAccess();
}

ry::I_StringA ry::Configuration::getJointNames(){
  return I_conv(K.get()->getJointNames());
}

pybind11::array ry::Configuration::getJointState(const I_StringA& joints){
  arr q;
  if(joints.size()) q = K.get()->getJointState(I_conv(joints));
  else q = K.get()->getJointState();
  return pybind11::array(q.dim(), q.p);
}

void ry::Configuration::setJointState(const std::vector<double>& q, const I_StringA& joints){
  arr _q = conv_stdvec2arr(q);
  if(joints.size()){
    K.set()->setJointState(_q, I_conv(joints));
  }else{
    K.set()->setJointState(_q);
  }
  rai::String str = "setJointState";
  _q.write(str,"\n");
  for(auto& d:cameras) d->update(str);
}

ry::I_StringA ry::Configuration::getFrameNames(){
  return I_conv(K.get()->getFrameNames());
}

pybind11::array ry::Configuration::getFrameState(){
  arr X = K.get()->getFrameState();
  return pybind11::array(X.dim(), X.p);
}

pybind11::array ry::Configuration::getFrameState(const char* frame){
  arr X;
  K.readAccess();
  rai::Frame *f = K().getFrameByName(frame, true);
  if(f) X=f->X.getArr7d();
  K.deAccess();
  return pybind11::array(X.dim(), X.p);
}

void ry::Configuration::setFrameState(const std::vector<double>& X, const I_StringA& frames, bool calc_q_from_X){
  arr _X = conv_stdvec2arr(X);
  _X.reshape(_X.N/7, 7);
  K.set()->setFrameState(_X, I_conv(frames), calc_q_from_X);
  for(auto& d:cameras) d->update(STRING("setFrameState"));
}

void ry::Configuration::stash(){
  arr X = K.get()->getFrameState();
  stack.append(X);
  stack.reshape(stack.N/X.N, X.d0, 7);
}

void ry::Configuration::pop(){
  arr X = stack[-1];
  stack.resizeCopy(stack.d0-1, stack.d1, stack.d2);
  K.set()->setFrameState(X);
  for(auto& d:cameras) d->update(STRING("pop"));
}

void ry::Configuration::useJointGroups(const ry::I_StringA& jointGroups){
  K.set()->useJointGroups(I_conv(jointGroups), true, true);
}

void ry::Configuration::setActiveJoints(const ry::I_StringA& joints) {
  // TODO: this is joint groups
  // TODO: maybe call joint groups just joints and joints DOFs
  K.set()->setActiveJointsByName(I_conv(joints));
}

void ry::Configuration::makeObjectsFree(const ry::I_StringA& objs){
  K.set()->makeObjectsFree(I_conv(objs));
}

double ry::Configuration::getPairDistance(const char* frameA, const char* frameB){
  K.readAccess();
  TM_PairCollision coll(K(), frameA, frameB, TM_PairCollision::_negScalar, false);
  arr y;
  coll.phi(y, NoArr, K());

  rai::Proxy& proxy = K().proxies.append();
  proxy.a = K().frames(coll.i);
  proxy.b = K().frames(coll.j);

  proxy.d = coll.coll->distance;
  proxy.normal = coll.coll->normal;
  arr P1 = coll.coll->p1;
  arr P2 = coll.coll->p2;
  if(coll.coll->rad1>0.) P1 -= coll.coll->rad1*coll.coll->normal;
  if(coll.coll->rad2>0.) P2 += coll.coll->rad2*coll.coll->normal;
  proxy.posA = P1;
  proxy.posB = P2;

  K.deAccess();
  for(auto& d:cameras) d->update(STRING("getPairDistance " <<frameA <<' ' <<frameB <<" = " <<-y.scalar()));
  return -y.scalar();
}

//ry::Camera ry::Configuration::display(){ return Camera(this); }

ry::Camera ry::Configuration::camera(const std::string& frame, bool _renderInBackground){
  return Camera(this, rai::String(frame), _renderInBackground);
}

ry::KOMOpy ry::Configuration::komo_IK(){
  return KOMOpy(this, 0);
}

ry::KOMOpy ry::Configuration::komo_path(double phases, uint stepsPerPhase, double timePerPhase){
  return KOMOpy(this, phases, stepsPerPhase, timePerPhase);
}

ry::KOMOpy ry::Configuration::komo_CGO(uint numConfigurations){
  CHECK_GE(numConfigurations, 1, "");
  return KOMOpy(this, numConfigurations);
}

ry::LGPpy ry::Configuration::lgp(const std::string& folFileName){
  return LGPpy(this, folFileName);
}


