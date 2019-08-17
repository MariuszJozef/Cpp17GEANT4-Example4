#include "MyPrimaryGeneratorAction.hh"
#include "MyPrimaryGeneratorMessenger.hh"

#include "G4Event.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "globals.hh"
#include "G4UnitsTable.hh"
#include "Randomize.hh"
#include "G4RunManager.hh"

using std::make_unique;

MyPrimaryGeneratorAction::MyPrimaryGeneratorAction(G4ThreeVector halfLabSize)
: G4VUserPrimaryGeneratorAction()
, myPrimaryGeneratorMessenger {make_unique<MyPrimaryGeneratorMessenger>(this)}
, particleGun {make_unique<G4ParticleGun>(1)}
, halfLabSize {halfLabSize}
, gunPosition {G4ThreeVector(0, 0, -halfLabSize.z())}
{
	G4ParticleTable *particleTable {G4ParticleTable::GetParticleTable()};
	G4String particleName;

	electron = particleTable->FindParticle(particleName="e-");
	positron = particleTable->FindParticle(particleName="e+");
	gamma = particleTable->FindParticle(particleName="gamma");
	proton = particleTable->FindParticle(particleName="proton");

	G4ParticleDefinition *particle {electron};

	particleGun->SetParticleDefinition(particle);
	particleGun->SetParticleEnergy(gunEnergy);
	particleGun->SetParticlePosition(gunPosition);
}

//MyPrimaryGeneratorAction::~MyPrimaryGeneratorAction() {}

void MyPrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent)
{
	particleGun->SetParticlePosition(gunPosition); // UI command: /gun/position ...

	if (beamDispersionOn) // UI command: /beam/dispersionOn ...
	{
		particleGun->SetParticleMomentumDirection(RandomiseDirection(gunDirection));
	}
	else
	{	// UI command: /gun/direction ...
		particleGun->SetParticleMomentumDirection(gunDirection);
	}

	particleGun->GeneratePrimaryVertex(anEvent);

	gunParticleName = particleGun->GetParticleDefinition()->GetParticleName();
	gunEnergyAndUnit = G4BestUnit(particleGun->GetParticleEnergy(), "Energy");
}


void MyPrimaryGeneratorAction::SetGunPosition(const G4ThreeVector& gunPosition)
{
	if (IsGunInsideLab(gunPosition))
	{
		this->gunPosition = gunPosition;
		// Takes effect upon: /run/beamOn
//		G4RunManager::GetRunManager()->ReinitializeGeometry();
		// Effective immediately; removes beam
//		G4RunManager::GetRunManager()->DefineWorldVolume(Construct());
	}
	else
	{
		G4cerr << "Gun position must be inside lab volume" << G4endl;
	}
}

G4bool MyPrimaryGeneratorAction::IsGunInsideLab(const G4ThreeVector& gunPosition) const
{
	if (  ( abs(gunPosition.x()) <= halfLabSize.x() ) &&
		  ( abs(gunPosition.y()) <= halfLabSize.y() ) &&
		  ( abs(gunPosition.z()) <= halfLabSize.z() )
		)
		return true;
	else
		return false;
}

const G4ThreeVector& MyPrimaryGeneratorAction::ObtainUnitThreeVector(const G4ThreeVector& aThreeVector)
{
	return aThreeVector.unit();
}

void MyPrimaryGeneratorAction::SetGunDirection(const G4ThreeVector& gunDirection)
{
	G4bool aimGunAtOrigin { gunDirection == G4ThreeVector() ? true : false };
	if (aimGunAtOrigin)
	{
		// Aim gun at origin when gunPosition != (0, 0, 0) but the new gunDirection == (0, 0, 0)
		if (gunPosition != G4ThreeVector())
		{
			this->gunDirection = -ObtainUnitThreeVector(gunPosition);
		}
		else
		{
			G4cerr << "Cannot aim gun at (0, 0, 0) since gunPosition = (0, 0, 0); "
				   << "gun direction remains at: " << GetGunDirection() << G4endl;
		}
	}
	// User specified new gunDirection != (0, 0, 0) so aim gun there irrespective of gunPosition
	else
	{
		this->gunDirection = ObtainUnitThreeVector(gunDirection);
	}
}

void MyPrimaryGeneratorAction::SetGunDirectionAimAtPosition(const G4ThreeVector& aimedPosition)
{
	if (aimedPosition == gunPosition)
	{
		G4cerr << "Cannot aim gun at current gunPosition: "
			   << G4BestUnit(GetGunPosition(), "Length")
			   << "; gun direction unchanged" << G4endl;
	}
	else
	{
		gunDirection = ObtainUnitThreeVector(aimedPosition - gunPosition);
	}
}

const G4ThreeVector& MyPrimaryGeneratorAction::RandomiseDirection(const G4ThreeVector& gunDirection)
{
	G4ThreeVector randomiseDirection = G4ThreeVector();

	if (randomDistribution == "u" || randomDistribution == "uniform")
	{
		randomiseDirection = G4ThreeVector(
				G4UniformRand(), G4UniformRand(), G4UniformRand());
		// Shift these random numbers from (0, 1) interval to (-0.5, 0.5)
		randomiseDirection -= G4ThreeVector(0.5, 0.5, 0.5);
		randomiseDirection *= amplitudeOrSigma;
	}
	else if (randomDistribution == "g" || randomDistribution == "gaussian"
			|| randomDistribution == "Gaussian")
	{
		randomiseDirection = G4ThreeVector(
			G4RandGauss::shoot(0, amplitudeOrSigma),
			G4RandGauss::shoot(0, amplitudeOrSigma),
			G4RandGauss::shoot(0, amplitudeOrSigma));
	}
	return gunDirection + randomiseDirection;
}

void MyPrimaryGeneratorAction::SetBeamDistribution(
		const G4String& randomDistribution, G4double amplitudeOrSigma)
{
	this->randomDistribution = randomDistribution;
	this->amplitudeOrSigma = amplitudeOrSigma;
}

