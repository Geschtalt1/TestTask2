

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

	// Включаем репликацию переменной.
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

		// Проигрываем звук выстрела и вспышку.
		PlaySoundFire();
		SpawnEffectMuzzle();

		// Отнимаем один патрон.
		SetAmmoCurrent(GetAmmoCurrent() - 1);

		// Трейс стрельбы.
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
		// Проверяем режим огня.
		if (Gun.FireMode == EFireMode::FM_Auto)
		{
			// Если автоматический огонь, зацикливаем стрельбу.
			DELAY_TIMER(TimerFire, Gun.FireRate, &AGunBase::OnFire, true);
		}
		else
		{
			// Если стоит одиночный запускаем стрельбу сразу.
			OnFire();
			OnStopFire();
		}
	}
	else
	{
		// Отключаем стрельбу.
		OnStopFire();
	}
}

void AGunBase::OnStopFire()
{
	// Очищаем таймер стрельбы.
	GetWorld()->GetTimerManager().ClearTimer(TimerFire);

	if (IsFire())
	{
		bFire = false;

		// Запрещаем стрельбу.
		SetAllowedShoot(false);

		// Запускаем таймер, через время которого пистолет может снова стрелять.
		DELAY_OneParam(Gun.FireRate, &AGunBase::SetAllowedShoot, true);

		OnEndFire.Broadcast();
	}
}

void AGunBase::OnReload()
{
	// Если магазин с патронами не полный.
	if (GetAmmoCurrent() != Gun.Ammo.AmmoMaxInMag)
	{
		// Находим сколько патронов нужно добавить.
		const int32 FindAddAmmo = Gun.Ammo.FindAddAmmo();

		// Проверяем что общих патронов больше, требуемого для перезарядки.
		if (GetAmmoTotal() >= FindAddAmmo)
		{
			SetAmmoCurrent(GetAmmoCurrent() + FindAddAmmo);
			SetAmmoTotal(GetAmmoTotal() - FindAddAmmo);
		}
		else // Если общих патронов меньше.
		{
			// Добавляем к текущем патроном сколько осталось общих.
			int32 NewAmmo = GetAmmoCurrent() + GetAmmoTotal();

			// Обновляем патроны.
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
		// Спавн эффекта на стволе.
		auto muzzleLocal = UNiagaraFunctionLibrary::SpawnSystemAttached(
			Gun.MuzzleEffect,                         // Ссылка на эффект.
			GunMesh,                                  // Ссылка на мешь.
			MUZZLE_SOCKET,                            // Названия сокета где савниться эффект.
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
		// Записываем стартовкую позицию камеры.
		Start = GetCameraOwner()->GetComponentLocation();

		// Высчитываем конечную.
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
	// Координаты для спавна трейса.
	FVector StartLoc;
	FVector EndLoc;

	CalculateTraceFromCamera(StartLoc, EndLoc);

	FHitResult Result;

	// Создаем лайн трейс.
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
	// Если трейс попал во что-то.
	if (Hit.bBlockingHit)
	{
		// Наносим точечный урон.
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