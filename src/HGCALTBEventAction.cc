//**************************************************
// \file HGCALTBEventAction.cc
// \brief: Implementation of HGCALTBEventAction
//         class
// \author: Lorenzo Pezzotti (CERN EP-SFT-sim)
//          @lopezzot
// \start date: 11 January 2024
//**************************************************

// Includers from project files
//
#include "HGCALTBEventAction.hh"

#include "HGCALTBAHCALSD.hh"
#include "HGCALTBCEESD.hh"
#include "HGCALTBCHESD.hh"
#include "HGCALTBRunAction.hh"

// Includers from Geant4
//
#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4UnitsTable.hh"
#include "G4Version.hh"
#if G4VERSION_NUMBER < 1100
#  include "g4root.hh"
#else
#  include "G4AnalysisManager.hh"
#endif
#include "G4SDManager.hh"
#ifdef USE_CELERITAS
#include "Celeritas.hh"
#endif

// Includers from std
//
#include <numeric>

// constructor and de-constructor
//
HGCALTBEventAction::HGCALTBEventAction() : G4UserEventAction(), edep(0.), fIntLayer(0)
{
  fCEELayerSignals = std::vector<G4double>(HGCALTBConstants::CEELayers, 0.);
  fCHELayerSignals = std::vector<G4double>(HGCALTBConstants::CHELayers, 0.);
  fAHCALLayerSignals = std::vector<G4double>(HGCALTBConstants::AHCALLayers, 0.);
}

HGCALTBEventAction::~HGCALTBEventAction() {}

// Define BeginOfEventAction() and EndOfEventAction() virtual methods
//
void HGCALTBEventAction::BeginOfEventAction(const G4Event* event)
{
  // Initialize variables per event
  //
  edep = 0.;
  fIntLayer = 0;
  for (auto& value : fCEELayerSignals) {
    value = 0.;
  }
  for (auto& value : fCHELayerSignals) {
    value = 0.;
  }
  for (auto& value : fAHCALLayerSignals) {
    value = 0.;
  }
#ifdef USE_CELERITAS
  CelerSimpleOffload().BeginOfEventAction(event);
#else
  (void)sizeof(event);
#endif
}

// GetCEEHitsCollection method()
//
HGCALTBCEEHitsCollection* HGCALTBEventAction::GetCEEHitsCollection(G4int hcID,
                                                                   const G4Event* event) const
{
  auto hitsCollection =
    static_cast<HGCALTBCEEHitsCollection*>(event->GetHCofThisEvent()->GetHC(hcID));

  if (!hitsCollection) {
    G4ExceptionDescription msg;
    msg << "Cannot access hitsCollection ID " << hcID;
    G4Exception("HGCALTBEventAction::GetHitsCollection()", "MyCode0003", FatalException, msg);
  }

  return hitsCollection;
}

// GetCEEHitsCollection method()
//
HGCALTBCHEHitsCollection* HGCALTBEventAction::GetCHEHitsCollection(G4int hcID,
                                                                   const G4Event* event) const
{
  auto hitsCollection =
    static_cast<HGCALTBCHEHitsCollection*>(event->GetHCofThisEvent()->GetHC(hcID));

  if (!hitsCollection) {
    G4ExceptionDescription msg;
    msg << "Cannot access hitsCollection ID " << hcID;
    G4Exception("HGCALTBEventAction::GetHitsCollection()", "MyCode0003", FatalException, msg);
  }

  return hitsCollection;
}

// GetAHCALHitsCollection method()
//
HGCALTBAHCALHitsCollection* HGCALTBEventAction::GetAHCALHitsCollection(G4int hcID,
                                                                       const G4Event* event) const
{
  auto hitsCollection =
    static_cast<HGCALTBAHCALHitsCollection*>(event->GetHCofThisEvent()->GetHC(hcID));

  if (!hitsCollection) {
    G4ExceptionDescription msg;
    msg << "Cannot access hitsCollection ID " << hcID;
    G4Exception("HGCALTBEventAction::GetHitsCollection()", "MyCode0003", FatalException, msg);
  }

  return hitsCollection;
}

void HGCALTBEventAction::EndOfEventAction(const G4Event* event)
{
#ifdef USE_CELERITAS
  CelerSimpleOffload().EndOfEventAction(event);
#endif

  // Access Event random seeds
  //
  // auto rndseed = G4RunManager::GetRunManager()->GetRandomNumberStatusForThisEvent();

  // CEE Hits
  //
  auto CEEHCID =
    G4SDManager::GetSDMpointer()->GetCollectionID(HGCALTBCEESD::fCEEHitsCollectionName);
  HGCALTBCEEHitsCollection* CEEHC = GetCEEHitsCollection(CEEHCID, event);

  // lambda to apply calibration, add noise and apply a 0.5 MIP cut to CEE and CHE cells
  auto ApplyHGCALCut = [](G4double partialsum, G4double signal) -> G4double {
    auto calibsignal = signal / HGCALTBConstants::MIPSilicon;  // MIP calibration
    calibsignal += G4RandGauss::shoot(0., HGCALTBConstants::CEENoiseSigma);  // Noise
    if (calibsignal > HGCALTBConstants::CEEThreshold)  // Cut
      return partialsum + calibsignal;
    else
      return partialsum;
  };

  for (std::size_t i = 0; i < HGCALTBConstants::CEELayers; i++) {
    auto CEESignals = (*CEEHC)[i]->GetCEESignals();
    G4double CEELayerSignal =
      std::accumulate(CEESignals.begin(), CEESignals.end(), 0., ApplyHGCALCut);
    fCEELayerSignals[i] = CEELayerSignal;
  }

  // CHE Hits
  //
  auto CHEHCID =
    G4SDManager::GetSDMpointer()->GetCollectionID(HGCALTBCHESD::fCHEHitsCollectionName);
  HGCALTBCHEHitsCollection* CHEHC = GetCHEHitsCollection(CHEHCID, event);

  for (std::size_t i = 0; i < HGCALTBConstants::CHELayers; i++) {
    auto CHESignals = (*CHEHC)[i]->GetCHESignals();
    G4double CHELayerSignal = 0.;
    if (i < HGCALTBConstants::CHESevenWaferLayers) {
      CHELayerSignal = std::accumulate(CHESignals.begin(), CHESignals.end(), 0., ApplyHGCALCut);
    }
    else {  // for last 3 CHE layers only add up cells for 1 silicon wafer
      CHELayerSignal =
        std::accumulate(CHESignals.begin(), CHESignals.begin() + (HGCALTBConstants::CEECells - 1),
                        0., ApplyHGCALCut);
    }
    fCHELayerSignals[i] = CHELayerSignal;
  }

  // AHCAL Hits
  //
  auto AHCALHCID =
    G4SDManager::GetSDMpointer()->GetCollectionID(HGCALTBAHCALSD::fAHCALHitsCollectionName);
  HGCALTBAHCALHitsCollection* AHCALHC = GetAHCALHitsCollection(AHCALHCID, event);

  // lambda to apply calibration, add noise and apply a 0.5 MIP cut to AHCAL cells
  auto ApplyAHCut = [MIPTile = HGCALTBConstants::MIPTile,
                     AHThreshold = HGCALTBConstants::AHCALThreshold](G4double partialsum,
                                                                     G4double signal) -> G4double {
    auto calibsignal = signal / MIPTile;  // MIP calibration
    calibsignal += G4RandGauss::shoot(0., HGCALTBConstants::AHCALNoiseSigma);  // Noise
    if (calibsignal > AHThreshold)  // Cut
      return partialsum + calibsignal;
    else
      return partialsum;
  };

  for (std::size_t i = 0; i < HGCALTBConstants::AHCALLayers; i++) {
    auto AHCALSignals = (*AHCALHC)[i]->GetAHSignals();
    G4double AHCALLayerSignal =
      std::accumulate(AHCALSignals.begin(), AHCALSignals.end(), 0., ApplyAHCut);
    fAHCALLayerSignals[i] = AHCALLayerSignal;
  }

  // Accumulate statistics
  //
  auto CEETot = std::accumulate(fCEELayerSignals.begin(), fCEELayerSignals.end(), 0.);
  auto CHETot = std::accumulate(fCHELayerSignals.begin(), fCHELayerSignals.end(), 0.);
  auto AHCALTot = std::accumulate(fAHCALLayerSignals.begin(), fAHCALLayerSignals.end(), 0.);
  auto HGCALTot = CEETot + CHETot + AHCALTot;
  auto analysisManager = G4AnalysisManager::Instance();
  analysisManager->FillNtupleDColumn(0, edep);
  analysisManager->FillNtupleDColumn(1, CEETot);
  analysisManager->FillNtupleDColumn(2, CHETot);
  analysisManager->FillNtupleDColumn(3, AHCALTot);
  analysisManager->FillNtupleDColumn(4, HGCALTot);
  analysisManager->FillNtupleIColumn(5, fIntLayer);
  analysisManager->AddNtupleRow();
}

//**************************************************
