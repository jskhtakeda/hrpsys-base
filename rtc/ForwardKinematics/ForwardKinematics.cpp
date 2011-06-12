// -*- C++ -*-
/*!
 * @file  ForwardKinematics.cpp
 * @brief null component
 * $Date$
 *
 * $Id$
 */

#include <rtm/CorbaNaming.h>
#include "ForwardKinematics.h"

#include "hrpModel/Link.h"
#include "hrpModel/ModelLoaderUtil.h"

typedef coil::Guard<coil::Mutex> Guard;

// Module specification
// <rtc-template block="module_spec">
static const char* nullcomponent_spec[] =
  {
    "implementation_id", "ForwardKinematics",
    "type_name",         "ForwardKinematics",
    "description",       "forward kinematics component",
    "version",           "1.0",
    "vendor",            "AIST",
    "category",          "example",
    "activity_type",     "DataFlowComponent",
    "max_instance",      "10",
    "language",          "C++",
    "lang_type",         "compile",
    // Configuration variables

    ""
  };
// </rtc-template>

ForwardKinematics::ForwardKinematics(RTC::Manager* manager)
  : RTC::DataFlowComponentBase(manager),
    // <rtc-template block="initializer">
    m_qIn("q", m_q),
    m_basePosIn("basePos", m_basePos),
    m_baseRpyIn("baseRpy", m_baseRpy),
    m_qRefIn("qRef", m_qRef),
    m_basePosRefIn("basePosRef", m_basePosRef),
    m_baseRpyRefIn("baseRpyRef", m_baseRpyRef),
    m_ForwardKinematicsServicePort("ForwardKinematicsService"),
    // </rtc-template>
    dummy(0)
{
}

ForwardKinematics::~ForwardKinematics()
{
}



RTC::ReturnCode_t ForwardKinematics::onInitialize()
{
  std::cout << m_profile.instance_name << ": onInitialize()" << std::endl;
  // <rtc-template block="bind_config">
  // Bind variables and configuration variable
  
  // </rtc-template>

  // Registration: InPort/OutPort/Service
  // <rtc-template block="registration">
  // Set InPort buffers
  addInPort("q", m_qIn);
  addInPort("basePos", m_basePosIn);
  addInPort("baseRpy", m_baseRpyIn);
  addInPort("qRef", m_qRefIn);
  addInPort("basePosRef", m_basePosRefIn);
  addInPort("baseRpyRef", m_baseRpyRefIn);

  // Set OutPort buffer
  
  // Set service provider to Ports
  m_ForwardKinematicsServicePort.registerProvider("service0", "ForwardKinematicsService", m_service0);
  addPort(m_ForwardKinematicsServicePort);
  m_service0.setComp(this);
  
  // Set service consumers to Ports
  
  // Set CORBA Service Ports
  
  // </rtc-template>

  RTC::Properties& prop = getProperties();

  m_refBody = new hrp::Body();
  RTC::Manager& rtcManager = RTC::Manager::instance();
  std::string nameServer = rtcManager.getConfig()["corba.nameservers"];
  int comPos = nameServer.find(",");
  if (comPos < 0){
      comPos = nameServer.length();
  }
  nameServer = nameServer.substr(0, comPos);
  RTC::CorbaNaming naming(rtcManager.getORB(), nameServer.c_str());
  if (!loadBodyFromModelLoader(m_refBody, prop["model"].c_str(), 
                               CosNaming::NamingContext::_duplicate(naming.getRootContext()))){
      std::cerr << "failed to load model[" << prop["model"] << "]" 
                << std::endl;
  }
  m_actBody = new hrp::Body(*m_refBody);
  hrp::Link *l = m_actBody->rootLink();
  l->p[0] = l->p[1] = l->p[2] = 0.0;
  l->R(0,0) = 1.0; l->R(0,1) = 0.0; l->R(0,2) = 0.0; 
  l->R(1,0) = 0.0; l->R(1,1) = 1.0; l->R(1,2) = 0.0; 
  l->R(2,0) = 0.0; l->R(2,1) = 0.0; l->R(2,2) = 1.0; 

  m_refLink = m_refBody->rootLink();
  m_actLink = m_actBody->rootLink();
  
  return RTC::RTC_OK;
}



/*
RTC::ReturnCode_t ForwardKinematics::onFinalize()
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t ForwardKinematics::onStartup(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t ForwardKinematics::onShutdown(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

RTC::ReturnCode_t ForwardKinematics::onActivated(RTC::UniqueId ec_id)
{
  std::cout << m_profile.instance_name<< ": onActivated(" << ec_id << ")" << std::endl;
  return RTC::RTC_OK;
}

RTC::ReturnCode_t ForwardKinematics::onDeactivated(RTC::UniqueId ec_id)
{
  std::cout << m_profile.instance_name<< ": onDeactivated(" << ec_id << ")" << std::endl;
  return RTC::RTC_OK;
}

RTC::ReturnCode_t ForwardKinematics::onExecute(RTC::UniqueId ec_id)
{
  //std::cout << m_profile.instance_name<< ": onExecute(" << ec_id << ")" << std::endl;

  coil::TimeValue tm(coil::gettimeofday());
  m_tm.sec  = tm.sec();
  m_tm.nsec = tm.usec() * 1000;

  if (m_qIn.isNew()) {
      m_qIn.read();
      for (int i=0; i<m_actBody->numJoints(); i++){
          m_actBody->joint(i)->q = m_q.data[i];
      }
  }

  if (m_basePosIn.isNew()){
      m_basePosIn.read();
      hrp::Link *root = m_actBody->rootLink();
      root->p[0] = m_basePos.data.x;
      root->p[1] = m_basePos.data.y;
      root->p[2] = m_basePos.data.z;
  }
      
  if (m_baseRpyIn.isNew()) {
      m_baseRpyIn.read();
      hrp::Vector3 rpy;
      rpy[0] = m_baseRpy.data.r;
      rpy[1] = m_baseRpy.data.p;
      // use reference yaw angle instead of estimated one
      rpy[2] = m_baseRpyRef.data.y;
      m_actBody->rootLink()->R = hrp::rotFromRpy(rpy);
  }

  if (m_qRefIn.isNew()) {
      m_qRefIn.read();
      for (int i=0; i<m_refBody->numJoints(); i++){
          m_refBody->joint(i)->q = m_qRef.data[i];
      }
  }

  if (m_basePosRefIn.isNew()){
      m_basePosRefIn.read();
      hrp::Link *root = m_refBody->rootLink();
      root->p[0] = m_basePosRef.data.x;
      root->p[1] = m_basePosRef.data.y;
      root->p[2] = m_basePosRef.data.z;
  }

  if (m_baseRpyRefIn.isNew()){
      m_baseRpyRefIn.read();
      hrp::Vector3 rpy;
      rpy[0] = m_baseRpyRef.data.r;
      rpy[1] = m_baseRpyRef.data.p;
      rpy[2] = m_baseRpyRef.data.y;
      m_refBody->rootLink()->R = hrp::rotFromRpy(rpy);

  }

  {
      Guard guard(m_bodyMutex);
      m_refBody->calcForwardKinematics();
      m_actBody->calcForwardKinematics();
  }

  return RTC::RTC_OK;
}

/*
RTC::ReturnCode_t ForwardKinematics::onAborting(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t ForwardKinematics::onError(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t ForwardKinematics::onReset(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t ForwardKinematics::onStateUpdate(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t ForwardKinematics::onRateChanged(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

::CORBA::Boolean ForwardKinematics::getReferencePose(const char* linkname, RTC::TimedDoubleSeq_out pose)
{
    pose = new RTC::TimedDoubleSeq();
    Guard guard(m_bodyMutex);
    hrp::Link *l = m_refBody->link(linkname);
    if (!l) return false;
    const hrp::Vector3& p = l->p;
    const hrp::Matrix33& R = l->R;
    pose->tm = m_tm;
    pose->data.length(16);
    pose->data[ 0]=R(0,0);pose->data[ 1]=R(0,1);pose->data[ 2]=R(0,2);pose->data[ 3]=p[0];
    pose->data[ 4]=R(1,0);pose->data[ 5]=R(1,1);pose->data[ 6]=R(1,2);pose->data[ 7]=p[1];
    pose->data[ 8]=R(2,0);pose->data[ 9]=R(2,1);pose->data[10]=R(2,2);pose->data[11]=p[2];
    pose->data[12]=0;     pose->data[13]=0;     pose->data[14]=0;     pose->data[15]=1;
    return true;
}

::CORBA::Boolean ForwardKinematics::getCurrentPose(const char* linkname, RTC::TimedDoubleSeq_out pose)
{
    pose = new RTC::TimedDoubleSeq();
    Guard guard(m_bodyMutex);
    hrp::Link *l = m_actBody->link(linkname);
    if (!l) return false;
    hrp::Vector3 dp(m_refLink->p - m_actLink->p);

    const hrp::Vector3 p(l->p + dp);
    const hrp::Matrix33 &R = l->R;
    pose->tm = m_tm;
    pose->data.length(16);
    pose->data[ 0]=R(0,0);pose->data[ 1]=R(0,1);pose->data[ 2]=R(0,2);pose->data[ 3]=p[0];
    pose->data[ 4]=R(1,0);pose->data[ 5]=R(1,1);pose->data[ 6]=R(1,2);pose->data[ 7]=p[1];
    pose->data[ 8]=R(2,0);pose->data[ 9]=R(2,1);pose->data[10]=R(2,2);pose->data[11]=p[2];
    pose->data[12]=0;     pose->data[13]=0;     pose->data[14]=0;     pose->data[15]=1;
    return true;
}

::CORBA::Boolean ForwardKinematics::selectBaseLink(const char* linkname)
{
    Guard guard(m_bodyMutex);
    hrp::Link *l = m_refBody->link(linkname);
    if (!l) return false;
    m_refLink = l;
    m_actLink = m_actBody->link(linkname);
    return true;
}

extern "C"
{

  void ForwardKinematicsInit(RTC::Manager* manager)
  {
    RTC::Properties profile(nullcomponent_spec);
    manager->registerFactory(profile,
                             RTC::Create<ForwardKinematics>,
                             RTC::Delete<ForwardKinematics>);
  }

};


