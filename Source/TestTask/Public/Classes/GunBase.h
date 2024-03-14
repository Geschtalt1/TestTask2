
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "GunBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBeginFireSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEndFireSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAmmoNullSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFireHitResultSignature, FHitResult, HitInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAmmoNewCurrentSignature, int32, NewAmmoCurrent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAmmoNewTotalSignature, int32, NewAmmoTotal);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReloadingSignature, UAnimMontage*, Animation);

class UNiagaraSystem;
class UAnimationAsset;
class UAnimMontage;
class USoundCue;
class UCurveFloat;

/** Режим огня. */
UENUM(BlueprintType)
enum class EFireMode : uint8
{
	FM_Single         UMETA(DisplayName = "Single"),
	FM_Auto           UMETA(DisplayName = "Auto")
};

/** Кобура в которой будет находиться пистолет. */
UENUM(BlueprintType)
enum class EHolster : uint8
{
	EH_None     UMETA(DisplayName = "None"),
	EH_Main     UMETA(DisplayName = "Main"),
	EH_Second   UMETA(DisplayName = "Second")
};

/** Структура патронов. */
USTRUCT(BlueprintType)
struct FAmmo
{
	GENERATED_BODY()

public:
	FAmmo()
		: AmmoMaxInMag(0)
		, AmmoCurrentInMag(0)
		, AmmoTotalMax(0)
		, AmmoTotalCurrent(0)
	{}

	/** Максимальное кол - во патронов. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (NoResetToDefault, ClampMin = 0))
	int32 AmmoMaxInMag = 0;

	/** Текущее кол - во патронов в магазине. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (NoResetToDefault, ClampMin = 0))
	int32 AmmoCurrentInMag = 0;

	/** Максимальное кол - во общих патронов. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (NoResetToDefault, ClampMin = 0))
	int32 AmmoTotalMax = 0;

	/** Текущее кол - во общих патронов. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (NoResetToDefault, ClampMin = 0))
	int32 AmmoTotalCurrent = 0;

public:
	/** Возращает кол-во патрон, сколько не хватает в текущий момент в магазине. */
	FORCEINLINE int32 FindAddAmmo() const { return AmmoMaxInMag - AmmoCurrentInMag; }

};

/** Структура пистолета. */
USTRUCT(BlueprintType)
struct FGun : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

public:
	/** Мешь пистолета. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (NoResetToDefault))
	TSoftObjectPtr<USkeletalMesh> Mesh = nullptr;

	/** Патроны. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (NoResetToDefault))
	FAmmo Ammo;

	/** Урон пистолета. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (NoResetToDefault, ClampMin = 0))
	float Damage = 0.0f;

	/** Скорострельность. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (NoResetToDefault, ClampMin = 0))
	float FireRate = 0.0f;

	/** Разброс. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (NoResetToDefault, ClampMin = 0))
	float Spread = 0.0f;

	/** Режим стрельбы. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (NoResetToDefault))
	EFireMode FireMode = EFireMode::FM_Single;

	/** Коллизия луча. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (NoResetToDefault))
	TEnumAsByte<ETraceTypeQuery> FireTrace = ETraceTypeQuery::TraceTypeQuery1;

	/** Сокет крепления. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (NoResetToDefault))
	FName SocketAttach = FName("weapon");

	/** Эффект выстрела. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (NoResetToDefault))
	TObjectPtr<UNiagaraSystem> MuzzleEffect = nullptr;

	/** Звук выстрела пистолета. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (NoResetToDefault))
	TObjectPtr<USoundCue> ShootSound = nullptr;
};

UCLASS(Abstract)
class TESTTASK_API AGunBase : public AActor
{
	GENERATED_BODY()
	
public:	
	/** Конструктор пистолета. */
	AGunBase(const FObjectInitializer& ObjectInitializer);

public:
	/**  */
	UFUNCTION(BlueprintCallable, Category = "Gun|Action")
	virtual void SetFire(bool bEnabled);

	/** Перезарядка пистолета. */
	UFUNCTION(BlueprintCallable, Category = "Gun|Action")
	virtual void OnReload();

	/** Выстрел из пистолета. */
	UFUNCTION(BlueprintCallable, Category = "Gun|Action")
	virtual void OnFire();

	/** Отключает стрельбу. */
	UFUNCTION(BlueprintCallable, Category = "Gun|Action")
	virtual void OnStopFire();

	/** Проверяет условия для выстрела. */
	UFUNCTION(BlueprintCallable, Category = "Gun|Condition")
	virtual bool OnPreFire() const;

	/** Устанавливает новое кол-во патронов в магазине. */
	UFUNCTION(BlueprintCallable, Category = "Gun|Ammo")
	void SetAmmoCurrent(int32 NewCurrent);

	/** Устанавливает общее кол - во патронов. */
	UFUNCTION(BlueprintCallable, Category = "Gun|Ammo")
	void SetAmmoTotal(int32 NewTotal);

	/** Рассчитывает трейс оружия. */
	UFUNCTION(BlueprintCallable, Category = "Gun|Trace")
	virtual bool CalculateFiringTrace();

	/** Создает трейс из камеры. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Gun|Trace")
	bool CalculateTraceFromCamera(FVector& Start, FVector& End);

	/** Рассчитывает разброс трейса. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Gun|Trace")
	FVector CalculateSpread(const FVector& InputTrace) const;

public:
	/** Возращает true, если в текущий момент патронов в маазине больше нуля. */
	UFUNCTION(BlueprintCallable, Category = "Gun|Ammo")
	FORCEINLINE bool IsAmmo() const { return GetAmmoCurrent() > 0; }

	/** Возращает текущее кол-во патронов в магазине. */
	UFUNCTION(BlueprintCallable, Category = "Gun|Ammo")
	FORCEINLINE int32 GetAmmoCurrent() const { return Gun.Ammo.AmmoCurrentInMag; }

	/** Возращает общее кол-во патронов для текущего пистолета. */
	UFUNCTION(BlueprintCallable, Category = "Gun|Ammo")
	FORCEINLINE int32 GetAmmoTotal() const { return Gun.Ammo.AmmoTotalCurrent; }

public:
	/** Возращает ссылку на настройку пистолета. */
	FORCEINLINE const FGun& GetGun() const { return Gun; }

	/** Возращает true, если в текущий момент пистолет стреляет. */
	FORCEINLINE bool IsFire() const { return bFire; }

	/** Возращает ссылку на мешь пистолета. */
	FORCEINLINE class USkeletalMeshComponent* GetMesh() const { return GunMesh; }

	/** Возращает камеру владельца. */
	class UCameraComponent* GetCameraOwner();

public:
	/** Делегат вызывается когда пистолет делает выстрел. */
	UPROPERTY(BlueprintReadWrite, BlueprintAssignable, BlueprintCallable, meta = (DisplayName = "On Begin Fire"))
	FOnBeginFireSignature OnBeginFire;

	/** Делегат вызывается когда пистолет прекратил стрелять. */
	UPROPERTY(BlueprintReadWrite, BlueprintAssignable, BlueprintCallable, meta = (DisplayName = "On End Fire"))
	FOnEndFireSignature OnEndFire;

	/** Делегат обновления патронов в оружие. */
	UPROPERTY(BlueprintReadWrite, BlueprintAssignable, BlueprintCallable, meta = (DisplayName = "On Ammo New Current"))
	FOnAmmoNewCurrentSignature OnAmmoNewCurrent;

	/** Делегат обновления общего кол-во патронов для оружия. */
	UPROPERTY(BlueprintReadWrite, BlueprintAssignable, BlueprintCallable, meta = (DisplayName = "On Ammo New Total"))
	FOnAmmoNewTotalSignature OnAmmoNewTotal;

	/** Делегат когда закончились патроны. */
	UPROPERTY(BlueprintReadWrite, BlueprintAssignable, BlueprintCallable, meta = (DisplayName = "On Ammo Null"))
	FOnAmmoNullSignature OnAmmoNull;

	/** Делегат перезарядки. */
	UPROPERTY(BlueprintReadWrite, BlueprintAssignable, BlueprintCallable, meta = (DisplayName = "On Reloading"))
	FOnReloadingSignature OnReloading;

	/** Делегат результат выстрела. */
	UPROPERTY(BlueprintReadWrite, BlueprintAssignable, BlueprintCallable, meta = (DisplayName = "On Fire Hit Result"))
	FOnFireHitResultSignature OnFireHitResult;

public:
	/** Включить дебаг трейса оружия. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Gun Setting|Advanced")
	uint8 bDrawDebug : 1;

public:
	/** Сокет ствола. */
	static const FName MUZZLE_SOCKET;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	/** Устанавливает доступ к стрельбе из пистолета. */
	void SetAllowedShoot(bool bEnabled);
	
	/** Проигрывает звук на скелете пистолета. */
	void PlaySoundFire();

	/** Спавн эффекта вспышки. */
	void SpawnEffectMuzzle();

	/** Проверяет куда попал трейс. */
	void CheckTraceHit(const FHitResult& Hit);

private:
	/** Скелет пистолета. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Component")
	TObjectPtr<class USkeletalMeshComponent> GunMesh;

private:
	/** Разрешено ли стрелять оружию.*/
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Gun Setting|Private")
	bool bAllowedShoot;

	/** Отслеживает стрельбу пистолета. */
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Gun Setting|Private")
	bool bFire;

private:
	/** Настройка пистолета. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, meta = (AllowPrivateAccess = "true", ExposeOnSpawn = "true"), Category = "Gun Setting")
	FGun Gun;

	/** Камера владельца. */
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Gun Setting|Private")
	TObjectPtr<class UCameraComponent> CameraOwner;

	/** Таймер стрельбы. */
	UPROPERTY()
	FTimerHandle TimerFire;
};
