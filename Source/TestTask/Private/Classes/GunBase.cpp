

#include "Classes/GunBase.h"
#include "Other/Macros.h"

#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

#include "Sound/SoundCue.h"
#include "Net/UnrealNetwork.h"

const FName AGunBase::MUZZLE_SOCKET = { "muzzle" };
AGunBase::AGunBase(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	GunMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	SetRootComponent(GunMesh);
	GunMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	bFire = false;
	bAllowedShoot = true;
	bDrawDebug = false;
}

void AGunBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// �������� ���������� ����������.
	DOREPLIFETIME(AGunBase, Gun);
}

void AGunBase::BeginPlay()
{
	Super::BeginPlay();
	
}

void AGunBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AGunBase::SetAmmoCurrent(int32 NewCurrent)
{
	Gun.Ammo.AmmoCurrentInMag = UKismetMathLibrary::Clamp(
		NewCurrent,
		0,
		Gun.Ammo.AmmoMaxInMag
	);

	OnAmmoNewCurrent.Broadcast(Gun.Ammo.AmmoCurrentInMag);

	if (GetAmmoCurrent() == 0) { OnAmmoNull.Broadcast(); }
}

void AGunBase::SetAmmoTotal(int32 NewTotal)
{
	Gun.Ammo.AmmoTotalCurrent = UKismetMathLibrary::Clamp(
		NewTotal,
		0,
		Gun.Ammo.AmmoTotalMax
	);

	OnAmmoNewTotal.Broadcast(Gun.Ammo.AmmoTotalCurrent);
}

void AGunBase::OnFire()
{
	if (OnPreFire())
	{
		bFire = true;

		// ����������� ���� �������� � �������.
		PlaySoundFire();
		SpawnEffectMuzzle();

		// �������� ���� ������.
		SetAmmoCurrent(GetAmmoCurrent() - 1);

		// ����� ��������.
		CalculateFiringTrace();

		OnBeginFire.Broadcast();
	}
}

bool AGunBase::OnPreFire() const
{
	return IsAmmo() && bAllowedShoot;
}

void AGunBase::SetFire(bool bEnabled)
{
	if (bEnabled)
	{
		// ��������� ����� ����.
		if (Gun.FireMode == EFireMode::FM_Auto)
		{
			// ���� �������������� �����, ����������� ��������.
			DELAY_TIMER(TimerFire, Gun.FireRate, &AGunBase::OnFire, true);
		}
		else
		{
			// ���� ����� ��������� ��������� �������� �����.
			OnFire();
			OnStopFire();
		}
	}
	else
	{
		// ��������� ��������.
		OnStopFire();
	}
}

void AGunBase::OnStopFire()
{
	// ������� ������ ��������.
	GetWorld()->GetTimerManager().ClearTimer(TimerFire);

	if (IsFire())
	{
		bFire = false;

		// ��������� ��������.
		SetAllowedShoot(false);

		// ��������� ������, ����� ����� �������� �������� ����� ����� ��������.
		DELAY_OneParam(Gun.FireRate, &AGunBase::SetAllowedShoot, true);

		OnEndFire.Broadcast();
	}
}

void AGunBase::OnReload()
{
	// ���� ������� � ��������� �� ������.
	if (GetAmmoCurrent() != Gun.Ammo.AmmoMaxInMag)
	{
		// ������� ������� �������� ����� ��������.
		const int32 FindAddAmmo = Gun.Ammo.FindAddAmmo();

		// ��������� ��� ����� �������� ������, ���������� ��� �����������.
		if (GetAmmoTotal() >= FindAddAmmo)
		{
			SetAmmoCurrent(GetAmmoCurrent() + FindAddAmmo);
			SetAmmoTotal(GetAmmoTotal() - FindAddAmmo);
		}
		else // ���� ����� �������� ������.
		{
			// ��������� � ������� �������� ������� �������� �����.
			int32 NewAmmo = GetAmmoCurrent() + GetAmmoTotal();

			// ��������� �������.
			SetAmmoCurrent(NewAmmo);
			SetAmmoTotal(NewAmmo * (-1));
		}
	}
}

void AGunBase::SetAllowedShoot(bool bEnabled)
{
	bAllowedShoot = bEnabled;
}

void AGunBase::PlaySoundFire()
{
	UGameplayStatics::SpawnSoundAttached(
		Gun.ShootSound,
		GunMesh,
		MUZZLE_SOCKET,
		GunMesh->GetSocketLocation(MUZZLE_SOCKET),
		EAttachLocation::SnapToTarget
	);
}

void AGunBase::SpawnEffectMuzzle()
{
	if (Gun.MuzzleEffect)
	{
		// ����� ������� �� ������.
		auto muzzleLocal = UNiagaraFunctionLibrary::SpawnSystemAttached(
			Gun.MuzzleEffect,                         // ������ �� ������.
			GunMesh,                                  // ������ �� ����.
			MUZZLE_SOCKET,                            // �������� ������ ��� ��������� ������.
			GunMesh->GetSocketLocation(MUZZLE_SOCKET),
			GunMesh->GetSocketRotation(MUZZLE_SOCKET),
			EAttachLocation::KeepWorldPosition,
			true
		);
	}
}

bool AGunBase::CalculateTraceFromCamera(FVector& Start, FVector& End)
{
	if (GetCameraOwner())
	{
		// ���������� ���������� ������� ������.
		Start = GetCameraOwner()->GetComponentLocation();

		// ����������� ��������.
		End = CalculateSpread(GetCameraOwner()->GetForwardVector() * 100000.0f) + Start;

		return true;
	}

	return false;
}

UCameraComponent* AGunBase::GetCameraOwner()
{
	if (!CameraOwner) {
		if (GetOwner()) {
			CameraOwner = GetOwner()->FindComponentByClass<UCameraComponent>();
		}
	}

	return CameraOwner;
}

FVector AGunBase::CalculateSpread(const FVector& InputTrace) const
{
	return InputTrace + FVector(
		UKismetMathLibrary::RandomFloatInRange(Gun.Spread * (-1), Gun.Spread),  // X
		UKismetMathLibrary::RandomFloatInRange(Gun.Spread * (-1), Gun.Spread),  // Y
		UKismetMathLibrary::RandomFloatInRange(Gun.Spread * (-1), Gun.Spread)   // Z
	);
}

bool AGunBase::CalculateFiringTrace()
{
	// ���������� ��� ������ ������.
	FVector StartLoc;
	FVector EndLoc;

	CalculateTraceFromCamera(StartLoc, EndLoc);

	FHitResult Result;

	// ������� ���� �����.
	const bool bHit = UKismetSystemLibrary::LineTraceSingle(
		this,
		StartLoc,
		EndLoc,
		Gun.FireTrace,
		true,
		{ this, GetOwner() },
		bDrawDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
		Result,
		true
	);

	CheckTraceHit(Result);
	OnFireHitResult.Broadcast(Result);

	return bHit;
}

void AGunBase::CheckTraceHit(const FHitResult& Hit)
{
	// ���� ����� ����� �� ���-��.
	if (Hit.bBlockingHit)
	{
		// ������� �������� ����.
		UGameplayStatics::ApplyPointDamage(
			Hit.GetActor(),
			Gun.Damage,
			FVector(),
			Hit,
			GetOwner() != nullptr ? GetOwner()->GetInstigatorController() : nullptr,
			GetOwner(),
			UDamageType::StaticClass()
		);
	}
}