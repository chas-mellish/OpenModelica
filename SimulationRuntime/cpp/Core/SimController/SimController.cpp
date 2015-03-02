#include <Core/Modelica.h>
#include <SimCoreFactory/Policies/FactoryConfig.h>
#include <Core/SimController/ISimController.h>
#include <Core/SimController/SimController.h>
#include <Core/SimController/Configuration.h>
#if defined(OMC_BUILD) || defined(SIMSTER_BUILD)
#include "LibrariesConfig.h"
#endif

SimController::SimController(PATH library_path, PATH modelicasystem_path)
    : SimControllerPolicy(library_path, modelicasystem_path, library_path)
    , _initialized(false)
{
  _config = boost::shared_ptr<Configuration>(new Configuration(_library_path, _config_path, modelicasystem_path));
  _algloopsolverfactory = createAlgLoopSolverFactory(_config->getGlobalSettings());
}

SimController::~SimController()
{
  _systems.clear();
}
/*
#if defined(__TRICORE__) || defined(__vxworks)
#else
std::pair<boost::shared_ptr<IMixedSystem>, boost::shared_ptr<ISimData> > SimController::LoadSystem(boost::shared_ptr<ISimData> (*createSimDataCallback)(), boost::shared_ptr<IMixedSystem> (*createSystemCallback)(IGlobalSettings*, boost::shared_ptr<IAlgLoopSolverFactory>, boost::shared_ptr<ISimData>), string modelKey)
{
  //if the model is already loaded
  std::map<string, boost::shared_ptr<IMixedSystem>  > ::iterator iter = _systems.find(modelKey);
  if(iter!=_systems.end())
  {
    //destroy system
    _systems.erase(iter);
  }
  //create system
  std::pair<boost::shared_ptr<IMixedSystem>, boost::shared_ptr<ISimData> > system = createSystem(createSimDataCallback, createSystemCallback, _config->getGlobalSettings(), _algloopsolverfactory);
  _systems[modelKey] = system;
  return system;
}
#endif
*/
boost::weak_ptr<IMixedSystem> SimController::LoadSystem(string modelLib,string modelKey)
{


  //if the model is already loaded
  std::map<string,boost::shared_ptr<IMixedSystem> > ::iterator iter = _systems.find(modelKey);
  if(iter!=_systems.end())
  {

     //destroy simdata
      std::map<string,boost::shared_ptr<ISimData> >::iterator iter2 = _sim_data.find(modelKey);
      if(iter2!=_sim_data.end())
      {
        _sim_data.erase(iter2);
      }
    //destroy system
    _systems.erase(iter);
     LoadSimData(modelKey);
  }
   boost::shared_ptr<ISimData> simData = getSimData(modelKey).lock();
  //create system
   boost::shared_ptr<IMixedSystem> system = createSystem(modelLib, modelKey, _config->getGlobalSettings(), _algloopsolverfactory,simData);
  _systems[modelKey] = system;
  return system;
}
boost::weak_ptr<IMixedSystem> SimController::LoadModelicaSystem(PATH modelica_path,string modelKey)
{
  if(_use_modelica_compiler)
  {

    //if the modell is already loaded
    std::map<string,boost::shared_ptr<IMixedSystem> >::iterator iter = _systems.find(modelKey);
    if(iter!=_systems.end())
    {
      //destroy simdata
      std::map<string,boost::shared_ptr<ISimData> >::iterator iter2 = _sim_data.find(modelKey);
      if(iter2!=_sim_data.end())
      {
        _sim_data.erase(iter2);
      }
       //destroy system
      _systems.erase(iter);
      LoadSimData(modelKey);
    }
    boost::shared_ptr<ISimData> simData = getSimData(modelKey).lock();
    boost::shared_ptr<IMixedSystem> system = createModelicaSystem(modelica_path, modelKey, _config->getGlobalSettings(), _algloopsolverfactory,simData);
    _systems[modelKey] = system;
    return system;
  }
  else
    throw ModelicaSimulationError(SIMMANAGER,"No Modelica Compiler configured");
}
boost::weak_ptr<ISimData> SimController::LoadSimData(string modelKey)
{
     //if the simdata is already loaded
  std::map<string,boost::shared_ptr<ISimData> > ::iterator iter = _sim_data.find(modelKey);
  if(iter!=_sim_data.end())
  {

     //destroy system
    _sim_data.erase(iter);

  }
  //create system
   boost::shared_ptr<ISimData> sim_data = createSimData();
  _sim_data[modelKey] = sim_data;
  return sim_data;




}


boost::weak_ptr<ISimData> SimController::getSimData(string modelname)
{


    std::map<string,boost::shared_ptr<ISimData> >::iterator iter = _sim_data.find(modelname);
    if(iter!=_sim_data.end())
    {
      return iter->second;
    }
    else
    {
     string error = string("Simulation data was not found for model: ") + modelname;
     throw ModelicaSimulationError(SIMMANAGER,error);
    }
}

 boost::weak_ptr<IMixedSystem> SimController::getSystem(string modelname)
 {

     std::map<string,boost::shared_ptr<IMixedSystem> >::iterator iter = _systems.find(modelname);
    if(iter!=_systems.end())
    {
      return iter->second;
    }
    else
    {
     string error = string("Simulation data was not found for model: ") + modelname;
     throw ModelicaSimulationError(SIMMANAGER,error);
    }

 }

// Added for real-time simulation using VxWorks and Bodas
void SimController::StartVxWorks(SimSettings simsettings,string modelKey)
{
//boost::shared_ptr<IMixedSystem> mixedsystem,
  try
  {
    boost::shared_ptr<IMixedSystem> mixedsystem = getSystem(modelKey).lock();
    IGlobalSettings* global_settings = _config->getGlobalSettings();

    global_settings->useEndlessSim(true);
    global_settings->setStartTime(simsettings.start_time);
    global_settings->setEndTime(simsettings.end_time);
    global_settings->sethOutput(simsettings.step_size);
    global_settings->setResultsFileName(simsettings.outputfile_name);
    global_settings->setSelectedLinSolver(simsettings.linear_solver_name);
    global_settings->setSelectedNonLinSolver(simsettings.nonlinear_solver_name);
    global_settings->setSelectedSolver(simsettings.solver_name);
    global_settings->setOutputFormat(simsettings.outputFormat);
    global_settings->setAlarmTime(simsettings.timeOut);
    global_settings->setLogType(simsettings.logType);
    global_settings->setOutputPointType(simsettings.outputPointType);

    _simMgr = boost::shared_ptr<SimManager>(new SimManager(mixedsystem, _config.get()));

    ISolverSettings* solver_settings = _config->getSolverSettings();
    solver_settings->setLowerLimit(simsettings.lower_limit);
    solver_settings->sethInit(simsettings.lower_limit);
    solver_settings->setUpperLimit(simsettings.upper_limit);
    solver_settings->setRTol(simsettings.tolerance);
    solver_settings->setATol(simsettings.tolerance);

    _simMgr->initialize();
  }
  catch( ModelicaSimulationError& ex)
  {
    string error = add_error_info(string("Simulation failed for ") + simsettings.outputfile_name,ex.what(),ex.getErrorID());
    printf("Fehler %s\n",error.c_str());
    throw ModelicaSimulationError(SIMMANAGER,error);
  }
}

// Added for real-time simulation using VxWorks and Bodas
void SimController::calcOneStep()
{
  _simMgr->runSingleStep();
}


void SimController::Start(SimSettings simsettings, string modelKey)
{

  try
  {

    boost::shared_ptr<IMixedSystem> mixedsystem = getSystem(modelKey).lock();

    IGlobalSettings* global_settings = _config->getGlobalSettings();

    global_settings->setStartTime(simsettings.start_time);
    global_settings->setEndTime(simsettings.end_time);
    global_settings->sethOutput(simsettings.step_size);
    global_settings->setResultsFileName(simsettings.outputfile_name);
    global_settings->setSelectedLinSolver(simsettings.linear_solver_name);
    global_settings->setSelectedNonLinSolver(simsettings.nonlinear_solver_name);
    global_settings->setSelectedSolver(simsettings.solver_name);
    global_settings->setOutputFormat(simsettings.outputFormat);
    global_settings->setLogType(simsettings.logType);
    global_settings->setAlarmTime(simsettings.timeOut);
    global_settings->setOutputPointType(simsettings.outputPointType);

    _simMgr = boost::shared_ptr<SimManager>(new SimManager(mixedsystem, _config.get()));

    ISolverSettings* solver_settings = _config->getSolverSettings();
    solver_settings->setLowerLimit(simsettings.lower_limit);
    solver_settings->sethInit(simsettings.lower_limit);
    solver_settings->setUpperLimit(simsettings.upper_limit);
    solver_settings->setRTol(simsettings.tolerance);
    solver_settings->setATol(simsettings.tolerance);
    _simMgr->initialize();

    _simMgr->runSimulation();

    boost::shared_ptr<IWriteOutput> writeoutput_system = boost::dynamic_pointer_cast<IWriteOutput>(mixedsystem);

    if((global_settings->getOutputFormat()==BUFFER) && writeoutput_system)
  {

       boost::shared_ptr<ISimData> simData = getSimData(modelKey).lock();
      //get history object to query simulation results
      IHistory* history = writeoutput_system->getHistory();
      //simulation results (output variables)
      ublas::matrix<double> Ro;
      //query simulation result otuputs
      history->getOutputResults(Ro);
      vector<string> output_names;
      history->getOutputNames(output_names);
      string name;
      int j=0;
      BOOST_FOREACH(name,output_names)
      {
        ublas::vector<double> o_j;
        o_j =ublas::row(Ro,j);
        simData->addOutputResults(name,o_j);
        j++;
      }
      vector<double> time_values = history->getTimeEntries();
      simData->addTimeEntries(time_values);
  }

  }
  catch(ModelicaSimulationError & ex)
  {

    string error = add_error_info(string("Simulation failed for ") + simsettings.outputfile_name,ex.what(),ex.getErrorID());
    throw ModelicaSimulationError(SIMMANAGER,error);
  }
}

void SimController::Stop()
{
  if(_simMgr)
  _simMgr->stopSimulation();
}
