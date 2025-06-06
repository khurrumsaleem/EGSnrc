/*
###############################################################################
#
#  EGSnrc egs++ ensdf headers
#  Copyright (C) 2015 National Research Council Canada
#
#  This file is part of EGSnrc.
#
#  EGSnrc is free software: you can redistribute it and/or modify it under
#  the terms of the GNU Affero General Public License as published by the
#  Free Software Foundation, either version 3 of the License, or (at your
#  option) any later version.
#
#  EGSnrc is distributed in the hope that it will be useful, but WITHOUT ANY
#  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
#  FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for
#  more details.
#
#  You should have received a copy of the GNU Affero General Public License
#  along with EGSnrc. If not, see <http://www.gnu.org/licenses/>.
#
###############################################################################
#
#  Author:          Reid Townson, 2016
#
#  Contributors:
#
###############################################################################
*/


/*! \file egs_ensdf.h
 *  \brief The ensdf library header file
 *  \RT
 *
 */

#ifndef EGS_ENSDF_
#define EGS_ENSDF_

#include "egs_libconfig.h"
#include "egs_functions.h"
#include "egs_math.h"
#include "egs_alias_table.h"
#include "egs_atomic_relaxations.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <map>

using namespace std;

template <class T> class Branch {
public:

    Branch() {}

    ~Branch() {
        for (typename vector<T *>::iterator it  = branchLeaves.begin();
                it!=branchLeaves.end(); it++) {
            (*it)->removeBranch();
        }
        branchLeaves.clear();
    }

    void addLeaf(T *leaf) {
        branchLeaves.push_back(leaf);
    }

    void removeLeaf(T *leaf) {
        branchLeaves.erase(std::remove(branchLeaves.begin(),
                                       branchLeaves.end(),
                                       leaf), branchLeaves.end());
    }

    vector<T *> getLeaves() const {
        return branchLeaves;
    }

    // A new == operator for this class
    bool operator==(const Branch<T> &rhs) const {
        for (typename vector<T *>::const_iterator it = branchLeaves.begin();
                it!=branchLeaves.end(); it++) {

            bool foundLeaf = false;
            for (typename vector<T *>::const_iterator irhs =
                        rhs.branchLeaves.begin();
                    irhs!=rhs.branchLeaves.end(); irhs++) {

                if (*irhs != 0 && *it != 0) {
                    if (*irhs == *it) {
                        foundLeaf = true;
                    }
                }
            }

            if (!foundLeaf) {
                return false;
            }
        }
        return true;
    }

protected:
    vector<T *> branchLeaves;
};

template <class T> class Leaf {
public:

    Leaf(T *existingBranch) {
        branch = existingBranch;
        if (branch) {
            branch->addLeaf(this);
        }
    }

    ~Leaf() {
        if (branch) {
            branch->removeLeaf(this);
        }
        branch = 0;
    }

    virtual T *getBranch() const {
        return branch;
    }

    void removeBranch() {
        branch = 0;
    }

    // A new == operator for this class
    bool operator== (const T &rhs) const {
        if (branch==0 && rhs.branch==0) {
            return true;
        }
        else if ((branch==0) && rhs.branch!=0) {
            return false;
        }
        else if ((branch!=0) && rhs.branch==0) {
            return false;
        }
        else if (branch!=0 && rhs.branch!=0) {
            return *branch == *(rhs.branch);
        }
    }

private:
    T *branch;
};

// The Record class
class Record {
public:
    Record();
    Record(vector<string> ensdf);
    virtual ~Record();
    vector<string> getRecords() const;

protected:
    double recordToDouble(int startPos, int endPos);
    string recordToString(int startPos, int endPos);
    double getTag(string searchString, string notAfter);
    double parseHalfLife(int startPos, int endPos);
    double parseStdUncertainty(string value, string stdUncertainty);
    string getStringAfter(string searchString, size_t len);

    // All the lines corresponding to this record type
    vector<string> lines;
};

// Comment Record
class CommentRecord : public Record {
public:
    CommentRecord(vector<string> ensdf);
    vector<string> getComments();

private:
    vector<string> comments;
    void processEnsdf();
};

// Parent Record
class ParentRecord : public Record, public Branch<Leaf<ParentRecord> > {
public:
    ParentRecord(vector<string> ensdf);
    double getHalfLife() const;
    double getQ() const;

protected:
    double  halfLife,
            Q;

private:
    void processEnsdf();
};

class ParentRecordLeaf : public Leaf<ParentRecord> {
public:
    ParentRecordLeaf(ParentRecord *myRecord);
    virtual ParentRecord *getParentRecord() const;
};

// Normalization Record
class NormalizationRecord : public Record, public
    Branch<Leaf<NormalizationRecord> >, public ParentRecordLeaf {
public:
    NormalizationRecord(vector<string> ensdf, ParentRecord *parent);
    double getRelativeMultiplier() const;
    double getTransitionMultiplier() const;
    double getBranchMultiplier() const;
    double getBetaMultiplier() const;
    EGS_AtomicRelaxations *getRelaxations() const;
    double getBindingEnergy(int shell) const;
    int getNShell() const;
    void relax(int shell,
               EGS_Float ecut, EGS_Float pcut,
               EGS_RandomGenerator *rndm, double &edep,
               EGS_SimpleContainer<EGS_RelaxationParticle> &particles);

protected:
    double normalizeRelative;
    double normalizeTransition;
    double normalizeBeta;
    double normalizeBranch;

private:
    void processEnsdf();
    EGS_AtomicRelaxations *relaxations;
    int nshell, Z;
};

class NormalizationRecordLeaf : public Leaf<NormalizationRecord> {
public:
    NormalizationRecordLeaf(NormalizationRecord *myRecord);
    virtual NormalizationRecord *getNormalizationRecord() const;
};

// Level Record
class EGS_EXPORT LevelRecord : public Record, public Branch<Leaf<LevelRecord> > {
public:
    LevelRecord();
    LevelRecord(vector<string> ensdf);
    void resetDisintegrationIntensity();
    void cumulDisintegrationIntensity(double disintIntensity);
    double getDisintegrationIntensity() const;
    void setLevelCanDecay(bool canDecay);
    bool levelCanDecay() const;
    double getEnergy() const;
    double getHalfLife() const;

protected:
    double disintegrationIntensity;
    double energy;
    double halfLife;
    bool canDecay;

private:
    void processEnsdf();
};

class LevelRecordLeaf : public Leaf<LevelRecord> {
public:
    LevelRecordLeaf(LevelRecord *myRecord);
    virtual LevelRecord *getLevelRecord() const;
};

// Generic beta record
class EGS_EXPORT BetaRecordLeaf : public Record, public ParentRecordLeaf, public
    NormalizationRecordLeaf, public LevelRecordLeaf {
public:
    BetaRecordLeaf(vector<string> ensdf, ParentRecord *myParent,
                   NormalizationRecord *myNormalization, LevelRecord *myLevel);

    virtual double getFinalEnergy() const = 0;
    virtual double getBetaIntensity() const = 0;
    virtual double getPositronIntensity() const {
        return 0;
    };
    virtual double getECIntensity() const {
        return 0;
    };
    virtual void relax(int shell,
                       EGS_Float ecut, EGS_Float pcut,
                       EGS_RandomGenerator *rndm, double &edep,
                       EGS_SimpleContainer<EGS_RelaxationParticle> &particles) {};
    virtual void setBetaIntensity(double newIntensity)  = 0;
    int getCharge() const;
    void incrNumSampled();
    EGS_I64 getNumSampled() const;
    unsigned short int getZ() const;
    unsigned short int getAtomicWeight() const;
    unsigned short int getForbidden() const;
    void setSpectrum(EGS_AliasTable *bspec);
    EGS_AliasTable *getSpectrum() const;
    vector<double> ecShellIntensity;

protected:
    EGS_I64 numSampled;
    double finalEnergy;
    double betaIntensity;
    int q;
    unsigned short int Z;
    unsigned short int A;
    unsigned short int forbidden;
    EGS_AliasTable *spectrum;
};

// Beta- record
class EGS_EXPORT BetaMinusRecord : public BetaRecordLeaf {
public:
    BetaMinusRecord(vector<string> ensdf, ParentRecord *myParent,
                    NormalizationRecord *myNormalization, LevelRecord *myLevel);

    double getFinalEnergy() const;
    double getBetaIntensity() const;
    double getBetaIntensityUnc() const;
    void setBetaIntensity(double newIntensity);

private:
    void processEnsdf();
    double betaIntensityUnc;
};

// Beta+ Record (and Electron Capture)
class EGS_EXPORT BetaPlusRecord : public BetaRecordLeaf {
public:
    BetaPlusRecord(vector<string> ensdf, ParentRecord *myParent,
                   NormalizationRecord *myNormalization, LevelRecord *myLevel);

    double getFinalEnergy() const;
    double getBetaIntensity() const;
    double getPositronIntensity() const;
    double getPositronIntensityUnc() const;
    double getECIntensityUnc() const;
    void setBetaIntensity(double newIntensity);
    void setPositronIntensity(double newIntensity);
    void relax(int shell,
               EGS_Float ecut, EGS_Float pcut,
               EGS_RandomGenerator *rndm, double &edep,
               EGS_SimpleContainer<EGS_RelaxationParticle> &particles);

protected:
    double  ecIntensity,
            positronIntensity,
            ecIntensityUnc,
            positronIntensityUnc;

private:
    void processEnsdf();
};

// Gamma record
class EGS_EXPORT GammaRecord : public Record, public ParentRecordLeaf,
    public NormalizationRecordLeaf, public LevelRecordLeaf {
public:
    GammaRecord(vector<string> ensdf, ParentRecord *myParent,
                NormalizationRecord *myNormalization,
                LevelRecord *myLevel);
    GammaRecord(GammaRecord *gamma);

    double getDecayEnergy() const;
    double getTransitionIntensity() const;
    double getGammaIntensity() const;
    double getGammaIntensityUnc() const;
    double getICIntensity() const;
    double getICIntensityUnc() const;
    double getIPIntensity() const;
    double getIPIntensityUnc() const;
    void setTransitionIntensity(double newIntensity);
    void setGammaIntensity(double newIntensity);
    void setICIntensity(double newIntensity);
    double getMultiTransitionProb() const;
    void setMultiTransitionProb(double newIntensity);
    int getCharge() const;
    LevelRecord *getFinalLevel() const;
    void setFinalLevel(LevelRecord *newLevel);
    void incrGammaSampled();
    void incrICSampled();
    void incrIPSampled();
    EGS_I64 getGammaSampled() const;
    EGS_I64 getICSampled() const;
    EGS_I64 getIPSampled() const;
    vector<double> icIntensity;
    double getBindingEnergy(int shell) const;
    void relax(int shell,
               EGS_Float ecut, EGS_Float pcut,
               EGS_RandomGenerator *rndm, double &edep,
               EGS_SimpleContainer<EGS_RelaxationParticle> &particles);

protected:
    EGS_I64 numGammaSampled, numICSampled, numIPSampled;
    double  decayEnergy;
    double  transitionIntensity,
            multipleTransitionProb,
            gammaIntensity,
            gammaIntensityUnc,
            icCoeff,
            icCoeffUnc,
            ipCoeff,
            ipCoeffUnc;
    int q;
    LevelRecord *finalLevel;

private:
    void processEnsdf();
};

// Alpha record
class EGS_EXPORT AlphaRecord : public Record, public ParentRecordLeaf, public
    NormalizationRecordLeaf, public LevelRecordLeaf {
public:
    AlphaRecord(vector<string> ensdf, ParentRecord *myParent,
                NormalizationRecord *myNormalization, LevelRecord *myLevel);

    double getFinalEnergy() const;
    double getAlphaIntensity() const;
    double getAlphaIntensityUnc() const;
    int getCharge() const;
    void setAlphaIntensity(double newIntensity);
    void incrNumSampled();
    EGS_I64 getNumSampled() const;

protected:
    EGS_I64 numSampled;
    double  finalEnergy,
            alphaIntensity,
            alphaIntensityUnc;
    int q;

private:
    void processEnsdf();
};

/*! \brief The ensdf class for reading ensdf format data files

\ingroup egspp_main

Parses decay spectrum file in ensdf format, and builds the decay scheme into an
object oriented tree-like structure. This decay structure is useful for
\ref EGS_RadionuclideSpectrum used by \ref EGS_RadionuclideSource.

Note: Uncertainties on values are ignored. The energies and intensities for various
emissions are taken as is. Very low probability emissions (<1e-10) are discarded.

When processing an ensdf file, only the following records are considered:
Comment, Parent, Normalization, Level, Beta-, EC / Beta+, Alpha, Gamma.

When the input '<code>atomic relaxations = ensdf</code>' in
\ref EGS_RadionuclideSpectrum, the
X-Ray fluorescence and Auger emissions are obtained from ensdf Comment records.
This assumes a particular format for the data currently used by the LNHB,
which may or may not be forward compatible. When using this option,
be aware that fluorescent photons and Auger emissions are modeled statistically.
In other
words, the relaxation emissions are not correlated with specific disintegration
events. If you are coincidence counting, use
'<code>atomic relaxations = eadl</code>'.

There are a few nuances to the data interpretation with '<code>atomic relaxations = ensdf</code>'.
If a single emission intensity value is present for a combination of lines (where
multiple energies are provided), then the average energy of the lines is used.
For example, in the
case below a single line of energy 97.4527 keV would be used with an emission
intensity of 0.57 per 100 disintegrations.
\verbatim
221FR T        96.815         |]                 XKB3
221FR T        97.474         |]  0.57     5     XKB1
221FR T        98.069         |]                 XKB5II
\endverbatim
If an energy and intensity are given for the "total" of several lines, it is
only used if the intensities of the individual lines are not provided.
For example, in the case below a single line of energy 14.0895 keV
would be used.
\verbatim
221FR T        10.38-17.799       18.7     9     XL (total)
221FR T        10.38                             XLL
221FR T        11.89-12.03                       XLA
221FR T        13.254                            XLC
221FR T        13.877-15.639                     XLB
221FR T        16.752-17.799                     XLG
\endverbatim

The ensdf class has been tested on radionuclide data from
<a href="http://www.nucleide.org/DDEP_WG/DDEPdata.htm">LNHB DDEP</a>.

*/

class EGS_EXPORT EGS_Ensdf {

public:

    /*! \brief Construct an ensdf object.
     *
     */
    EGS_Ensdf(const string nuclide, const string ensdf_filename="",
              const string relaxType="eadl", const bool allowMultiTrans=false, int verbosity=1);

    /*! \brief Destructor. */
    ~EGS_Ensdf();

    vector<Record * > getRecords() const;
    vector<BetaRecordLeaf *> getBetaRecords() const;
    vector<ParentRecord * > getParentRecords() const;
    vector<LevelRecord * > getLevelRecords() const;
    vector<AlphaRecord * > getAlphaRecords() const;
    vector<GammaRecord * > getGammaRecords() const;
    vector<GammaRecord * > getMetastableGammaRecords() const;
    vector<GammaRecord * > getUncorrelatedGammaRecords() const;
    vector<double > getXRayIntensities() const;
    vector<double > getXRayEnergies() const;
    vector<double > getAugerIntensities() const;
    vector<double > getAugerEnergies() const;

    string radionuclide;
    int verbose;
    string relaxationType;
    unsigned short int Z;
    double decayDiscrepancy;
    bool allowMultiTransition;

    void normalizeIntensities();

protected:

    unsigned short int findAtomicWeight(string element);
    void parseEnsdf(vector<string> ensdf);
    void buildRecords();

    void getEmissionsFromComments();

    ifstream ensdf_file;
    unsigned short int A;

    vector<Record * > myRecords;
    vector<CommentRecord * > myCommentRecords;
    vector<ParentRecord * > myParentRecords;
    vector<NormalizationRecord * > myNormalizationRecords;
    vector<LevelRecord * > myLevelRecords;
    vector<BetaRecordLeaf *> myBetaRecords;
    vector<BetaMinusRecord * > myBetaMinusRecords;
    vector<BetaPlusRecord * > myBetaPlusRecords;
    vector<AlphaRecord * > myAlphaRecords;
    vector<GammaRecord * > myGammaRecords;
    vector<GammaRecord * > myMetastableGammaRecords;
    vector<GammaRecord * > myUncorrelatedGammaRecords;

private:

    vector<vector<string> > recordStack;
    vector<string> commentLines;
    vector<double>  xrayEnergies,
           xrayIntensities,
           augerEnergies,
           augerIntensities;
    ParentRecord *previousParent;
};




#endif
