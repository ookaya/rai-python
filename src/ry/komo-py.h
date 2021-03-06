#pragma once

#include "types.h"

#include <KOMO/komo.h>
#include <pybind11/numpy.h>

namespace ry{

  struct Configuration;

  struct KOMOpy_self : KOMO{
    Configuration* kin=0;
    bool denseMode = true;
    Graph features;
    Skeleton S;

    KOMOpy_self(Configuration* _kin, uint T);
    KOMOpy_self(Configuration* _kin, double phases, uint stepsPerPhase=20, double timePerPhase=5.);
    ~KOMOpy_self();

    void setDiscreteOpt(uint k);

    /// set an objective
    void setObjective(const arr& times, ObjectiveType type, FeatureSymbol feat, const StringA& frames, const std::map<std::string, std::vector<double> >& parameters={});

    /// output the defined problem as a generic graph, that can also be displayed, saved and loaded
    Graph getProblemGraph(bool includeValues=false);

    /// getting output after the optimization
    arr getPose(uint t, const rai::String& name);
    arr getRelPose(uint t, const rai::String& from, const rai::String& to);

  private:
    Objective* setObjective(const arr& times, ObjectiveType type, Feature* feature, const arr& target=NoArr, double scale=1e1);
    Feature* symbols2feature(FeatureSymbol feat, const StringA& frames, const std::map<std::string, std::vector<double> >& parameters={});
  };

  struct KOMOpy{
    ptr<KOMOpy_self> self;

    KOMOpy(Configuration* _kin, uint T);
    KOMOpy(Configuration* _kin, double phases, uint stepsPerPhase, double timePerPhase);
    ~KOMOpy();

    //--
    void makeObjectsFree(const I_StringA& objs);
    void activateCollisionPairs(const std::vector<std::pair<std::string, std::string>>& collision_pairs);
    void deactivateCollisionPairs(const std::vector<std::pair<std::string, std::string>>& collision_pairs);
    void timeOptimization();

    void clearObjectives();

    //-- core methods to add objectives
    void addObjective(const std::vector<int>& confs, const std::vector<double>& timeInterval, const std::string& type, const std::string& feature, const I_StringA& frames={}, const std::vector<double>& scale={}, const std::vector<double>& target={}, I_args parameters={});
    void addObjectives(const I_features& features);

    //-- macros
    void add_grasp(int conf, const char* gripper, const char* object);
    void add_place(int conf, const char* object, const char* table);
    void add_StableRelativePose(const std::vector<int>& confs, const char* gripper, const char* object);
    void add_StablePose(const std::vector<int>& confs, const char* object);
    void add_resting(int conf1, int conf2, const char* object);
    void add_restingRelative(int conf1, int conf2, const char* object, const char* tableOrGripper);

    //-- basis of LGP
    void addSkeleton(const std::vector<double>& times, I_StringA symbols);
    void setSkeleton();
    void skeleton2bound();

    //-- optimize
    void optimize();

    //-- get results
    int getT();
    void getConfiguration(int t);
//    std::string getReport();
    Graph getProblemGraph();
    double getConstraintViolations();
    double getCosts();
  };

}
