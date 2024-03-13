

#include "Classes/GunBase.h"
#include "Other/Macros.h"

#include "Components/SkeletalMeshComponent.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

#include "Sound/SoundCue.h"

const FName AGunBase::MUZZLE_SOCKET = { "muzzle" };
const FName AGunBase::IRONSIGHT_SOCKET = { "ironsight" };
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
		PlaySoundAction(Gun.ShootSound, MUZZLE_SOCKET);
		SpawnEffectMuzzle();

		// Отнимаем один патрон.
		SetAmmoCurrent(GetAmmoCurrent() - 1);

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

void AGunBase::PlaySoundAction(USoundBase* const Sound, const FName SocketAttach)
{
	UGameplayStatics::SpawnSoundAttached(Sound, GunMesh, SocketAttach);
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