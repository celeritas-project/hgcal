//**************************************************
// \file HGCALTBEventAction.hh
// \brief: Definition of HGCALTBEventAction class
// \author: Lorenzo Pezzotti (CERN EP-SFT-sim)
//          @lopezzot
// \start date: 11 January 2024
//**************************************************

// Prevent including header multiple times
//
#ifndef HGCALTBEventAction_h
#  define HGCALTBEventAction_h 1

#  include <vector>

// Includers from Gean4
//
#  include "G4UserEventAction.hh"
#  include "globals.hh"

// Includers from project files
//
#  include "HGCALTBCEEHit.hh"
#  include "HGCALTBCHEHit.hh"
#  include "HGCALTBConstants.hh"

class HGCALTBEventAction : public G4UserEventAction
{
  public:
    HGCALTBEventAction();
    virtual ~HGCALTBEventAction();

    // virtual methods from base class
    virtual void BeginOfEventAction(const G4Event* event);
    virtual void EndOfEventAction(const G4Event* event);

    void Addedep(G4double stepedep);
    std::vector<G4double>& GetCEESignals() { return fCEELayerSignals; };
    std::vector<G4double>& GetCHESignals() { return fCHELayerSignals; };

  private:
    HGCALTBCEEHitsCollection* GetCEEHitsCollection(G4int hcID, const G4Event* event) const;
    HGCALTBCHEHitsCollection* GetCHEHitsCollection(G4int hcID, const G4Event* event) const;
    G4double edep;  // energy deposited in every volume
    std::vector<G4double> fCEELayerSignals;  // signals per CEE layer
    std::vector<G4double> fCHELayerSignals;  // signals per CHE layer
};

inline void HGCALTBEventAction::Addedep(G4double stepedep)
{
  edep += stepedep;
}

#endif  // HGCALTBEventAction_h 1

//**************************************************