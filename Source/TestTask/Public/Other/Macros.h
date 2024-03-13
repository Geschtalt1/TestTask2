
#pragma once

/** ����� ������� � ��������� ��� ����������. */
#define DELAY(dealy, function) \
    FTimerHandle __delayTimerHandle; \
    GetWorld()->GetTimerManager().SetTimer(__delayTimerHandle, this, function, (dealy), false, 0.0f);

/** ����� ������� � ��������� ��� ����������, � ��������� ��������. */
#define DELAY_TIMER(timer, dealy, function, loop) \
    GetWorld()->GetTimerManager().SetTimer(timer, this, function, (dealy), loop, 0.0f); 

/** ����� ������� � ��������� � ����� ����������. */
#define DELAY_OneParam(delay, function, value) \
    FTimerHandle __delayTimerHandleOneParam; \
    FTimerDelegate __delayTimerDelegateOneParam = FTimerDelegate::CreateUObject(this, function, value); \
    GetWorld()->GetTimerManager().SetTimer(__delayTimerHandleOneParam, __delayTimerDelegateOneParam, (delay), false);

/** ����� ������� � ��������� � ����� ����������, � ��������� ��������. */
#define DELAY_TIMER_OneParam(timer, delay, function, value) \
    FTimerDelegate __delayTimerDelegateOneParam = FTimerDelegate::CreateUObject(this, function, value); \
    GetWorld()->GetTimerManager().SetTimer(timer, __delayTimerDelegateOneParam, (delay), false);
