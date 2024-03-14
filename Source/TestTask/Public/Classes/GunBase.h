
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

/** ����� ����. */
UENUM(BlueprintType)
enum class EFireMode : uint8
{
	FM_Single         UMETA(DisplayName = "Single"),
	FM_Auto           UMETA(DisplayName = "Auto")
};

/** ������ � ������� ����� ���������� ��������. */
UENUM(BlueprintType)
enum class EHolster : uint8
{
	EH_None     UMETA(DisplayName = "None"),
	EH_Main     UMETA(DisplayName = "Main"),
	EH_Second   UMETA(DisplayName = "Second")
};

/** ��������� ��������. */
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

	/** ������������ ��� - �� ��������. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (NoResetToDefault, ClampMin = 0))
	int32 AmmoMaxInMag = 0;

	/** ������� ��� - �� �������� � ��������. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (NoResetToDefault, ClampMin = 0))
	int32 AmmoCurrentInMag = 0;

	/** ������������ ��� - �� ����� ��������. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (NoResetToDefault, ClampMin = 0))
	int32 AmmoTotalMax = 0;

	/** ������� ��� - �� ����� ��������. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (NoResetToDefault, ClampMin = 0))
	int32 AmmoTotalCurrent = 0;

public:
	/** ��������� ���-�� ������, ������� �� ������� � ������� ������ � ��������. */
	FORCEINLINE int32 FindAddAmmo() const { return AmmoMaxInMag - AmmoCurrentInMag; }

};

/** ��������� ���������. */
USTRUCT(BlueprintType)
struct FGun : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

public:
	/** ���� ���������. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (NoResetToDefault))
	TSoftObjectPtr<USkeletalMesh> Mesh = nullptr;

	/** �������. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (NoResetToDefault))
	FAmmo Ammo;

	/** ���� ���������. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (NoResetToDefault, ClampMin = 0))
	float Damage = 0.0f;

	/** ����������������. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (NoResetToDefault, ClampMin = 0))
	float FireRate = 0.0f;

	/** �������. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (NoResetToDefault, ClampMin = 0))
	float Spread = 0.0f;

	/** ����� ��������. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (NoResetToDefault))
	EFireMode FireMode = EFireMode::FM_Single;

	/** �������� ����. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (NoResetToDefault))
	TEnumAsByte<ETraceTypeQuery> FireTrace = ETraceTypeQuery::TraceTypeQuery1;

	/** ����� ���������. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (NoResetToDefault))
	FName SocketAttach = FName("weapon");

	/** ������ ��������. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (NoResetToDefault))
	TObjectPtr<UNiagaraSystem> MuzzleEffect = nullptr;

	/** ���� �������� ���������. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (NoResetToDefault))
	TObjectPtr<USoundCue> ShootSound = nullptr;
};

UCLASS(Abstract)
class TESTTASK_API AGunBase : public AActor
{
	GENERATED_BODY()
	
public:	
	/** ����������� ���������. */
	AGunBase(const FObjectInitializer& ObjectInitializer);

public:
	/**  */
	UFUNCTION(BlueprintCallable, Category = "Gun|Action")
	virtual void SetFire(bool bEnabled);

	/** ����������� ���������. */
	UFUNCTION(BlueprintCallable, Category = "Gun|Action")
	virtual void OnReload();

	/** ������� �� ���������. */
	UFUNCTION(BlueprintCallable, Category = "Gun|Action")
	virtual void OnFire();

	/** ��������� ��������. */
	UFUNCTION(BlueprintCallable, Category = "Gun|Action")
	virtual void OnStopFire();

	/** ��������� ������� ��� ��������. */
	UFUNCTION(BlueprintCallable, Category = "Gun|Condition")
	virtual bool OnPreFire() const;

	/** ������������� ����� ���-�� �������� � ��������. */
	UFUNCTION(BlueprintCallable, Category = "Gun|Ammo")
	void SetAmmoCurrent(int32 NewCurrent);

	/** ������������� ����� ��� - �� ��������. */
	UFUNCTION(BlueprintCallable, Category = "Gun|Ammo")
	void SetAmmoTotal(int32 NewTotal);

	/** ������������ ����� ������. */
	UFUNCTION(BlueprintCallable, Category = "Gun|Trace")
	virtual bool CalculateFiringTrace();

	/** ������� ����� �� ������. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Gun|Trace")
	bool CalculateTraceFromCamera(FVector& Start, FVector& End);

	/** ������������ ������� ������. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Gun|Trace")
	FVector CalculateSpread(const FVector& InputTrace) const;

public:
	/** ��������� true, ���� � ������� ������ �������� � ������� ������ ����. */
	UFUNCTION(BlueprintCallable, Category = "Gun|Ammo")
	FORCEINLINE bool IsAmmo() const { return GetAmmoCurrent() > 0; }

	/** ��������� ������� ���-�� �������� � ��������. */
	UFUNCTION(BlueprintCallable, Category = "Gun|Ammo")
	FORCEINLINE int32 GetAmmoCurrent() const { return Gun.Ammo.AmmoCurrentInMag; }

	/** ��������� ����� ���-�� �������� ��� �������� ���������. */
	UFUNCTION(BlueprintCallable, Category = "Gun|Ammo")
	FORCEINLINE int32 GetAmmoTotal() const { return Gun.Ammo.AmmoTotalCurrent; }

public:
	/** ��������� ������ �� ��������� ���������. */
	FORCEINLINE const FGun& GetGun() const { return Gun; }

	/** ��������� true, ���� � ������� ������ �������� ��������. */
	FORCEINLINE bool IsFire() const { return bFire; }

	/** ��������� ������ �� ���� ���������. */
	FORCEINLINE class USkeletalMeshComponent* GetMesh() const { return GunMesh; }

	/** ��������� ������ ���������. */
	class UCameraComponent* GetCameraOwner();

public:
	/** ������� ���������� ����� �������� ������ �������. */
	UPROPERTY(BlueprintReadWrite, BlueprintAssignable, BlueprintCallable, meta = (DisplayName = "On Begin Fire"))
	FOnBeginFireSignature OnBeginFire;

	/** ������� ���������� ����� �������� ��������� ��������. */
	UPROPERTY(BlueprintReadWrite, BlueprintAssignable, BlueprintCallable, meta = (DisplayName = "On End Fire"))
	FOnEndFireSignature OnEndFire;

	/** ������� ���������� �������� � ������. */
	UPROPERTY(BlueprintReadWrite, BlueprintAssignable, BlueprintCallable, meta = (DisplayName = "On Ammo New Current"))
	FOnAmmoNewCurrentSignature OnAmmoNewCurrent;

	/** ������� ���������� ������ ���-�� �������� ��� ������. */
	UPROPERTY(BlueprintReadWrite, BlueprintAssignable, BlueprintCallable, meta = (DisplayName = "On Ammo New Total"))
	FOnAmmoNewTotalSignature OnAmmoNewTotal;

	/** ������� ����� ����������� �������. */
	UPROPERTY(BlueprintReadWrite, BlueprintAssignable, BlueprintCallable, meta = (DisplayName = "On Ammo Null"))
	FOnAmmoNullSignature OnAmmoNull;

	/** ������� �����������. */
	UPROPERTY(BlueprintReadWrite, BlueprintAssignable, BlueprintCallable, meta = (DisplayName = "On Reloading"))
	FOnReloadingSignature OnReloading;

	/** ������� ��������� ��������. */
	UPROPERTY(BlueprintReadWrite, BlueprintAssignable, BlueprintCallable, meta = (DisplayName = "On Fire Hit Result"))
	FOnFireHitResultSignature OnFireHitResult;

public:
	/** �������� ����� ������ ������. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Gun Setting|Advanced")
	uint8 bDrawDebug : 1;

public:
	/** ����� ������. */
	static const FName MUZZLE_SOCKET;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	/** ������������� ������ � �������� �� ���������. */
	void SetAllowedShoot(bool bEnabled);
	
	/** ����������� ���� �� ������� ���������. */
	void PlaySoundFire();

	/** ����� ������� �������. */
	void SpawnEffectMuzzle();

	/** ��������� ���� ����� �����. */
	void CheckTraceHit(const FHitResult& Hit);

private:
	/** ������ ���������. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Component")
	TObjectPtr<class USkeletalMeshComponent> GunMesh;

private:
	/** ��������� �� �������� ������.*/
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Gun Setting|Private")
	bool bAllowedShoot;

	/** ����������� �������� ���������. */
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Gun Setting|Private")
	bool bFire;

private:
	/** ��������� ���������. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, meta = (AllowPrivateAccess = "true", ExposeOnSpawn = "true"), Category = "Gun Setting")
	FGun Gun;

	/** ������ ���������. */
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Gun Setting|Private")
	TObjectPtr<class UCameraComponent> CameraOwner;

	/** ������ ��������. */
	UPROPERTY()
	FTimerHandle TimerFire;
};
