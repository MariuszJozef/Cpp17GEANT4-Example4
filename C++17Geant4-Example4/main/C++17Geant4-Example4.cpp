#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "G4UIExecutive.hh"
#include "G4VisExecutive.hh"

#include "MyDetectorConstruction.hh"
#include "MyPhysicsList.hh"
#include "MyPrimaryGeneratorAction.hh"
#include "MyRunAction.hh"

#include "Randomize.hh"
#include "time.h"

using std::unique_ptr;
using std::make_unique;

int main(int argc, char** argv)
{
	unique_ptr<G4RunManager> runManager {make_unique<G4RunManager>()};

// Randomise the gun firing sequence, otherwise each simulation run would be the same.
	CLHEP::HepRandom::setTheEngine(make_unique<CLHEP::RanecuEngine>().release());
	G4long seed {time(nullptr)};
	CLHEP::HepRandom::setTheSeed(seed);

	unique_ptr<MyDetectorConstruction> myDetectorConstruction
			{make_unique<MyDetectorConstruction>()};
	runManager->SetUserInitialization(myDetectorConstruction.get());

	unique_ptr<MyPhysicsList> myPhysicsList {make_unique<MyPhysicsList>()};
	runManager->SetUserInitialization(myPhysicsList.get());

	unique_ptr<MyPrimaryGeneratorAction> myPrimaryGeneratorAction
	{make_unique<MyPrimaryGeneratorAction>
		(myDetectorConstruction->GetHalfLabSize())};
	runManager->SetUserAction(myPrimaryGeneratorAction.get());

	unique_ptr<MyRunAction> myRunAction
		{make_unique<MyRunAction>(myDetectorConstruction.get(),
								  myPrimaryGeneratorAction.get())};
	runManager->SetUserAction(myRunAction.get());

	unique_ptr<G4VisManager> visManager {make_unique<G4VisExecutive>()};
	visManager->Initialize();

	G4UImanager *uiManager {G4UImanager::GetUIpointer()};
	unique_ptr<G4UIExecutive> uiExecutive {make_unique<G4UIExecutive>(argc, argv)};

	uiManager->ApplyCommand("/control/execute vis-init.macro");
	uiExecutive->SessionStart();

	return 0;
}
