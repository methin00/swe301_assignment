# Watchdog ve Worker

## Derleme

Repo kökünde çalıştır:

```bash
make
```

Bu komut `watchdog` ve `worker` programlarını derler.

## Çalıştırma

```bash
./watchdog
```

Watchdog iki worker başlatır. Log dosyası, komutu çalıştırdığın dizindeki `watchdog.log` dosyasıdır.

## Sinyaller

Watchdog çalışırken başka bir terminalde, başlangıçta gösterilen PID’yi kullan:

```bash
kill -USR1 <WATCHDOG_PID>
```

Bu komut çalışan worker’ların durumunu log dosyasına kaydeder.

Yalnızca watchdog’a SIGTERM göndermek için:

```bash
kill -TERM <WATCHDOG_PID>
```

Watchdog kapanırken worker süreçlerini durdurup toplar.

Ön planda çalışan watchdog’u `Ctrl+C` ile de durdurabilirsin. Denemede Ctrl+C, watchdog ile birlikte worker’lara da SIGINT gönderdi; worker’lar sinyal 2 ile sonlandı.

## Testleri çalıştırma

```bash
./tests/test_scenarios.sh
```

Test betiği iki worker’ın başlamasını, SIGUSR1 durum raporunu ve watchdog’a gönderilen SIGTERM/SIGINT sonrasında worker’ların kapanmasını kontrol eder. Örnek log `docs/sample_watchdog.log` dosyasındadır.

Normal çıkış, beklenmedik çıkış ve üç yeniden başlatma sonrasındaki dördüncü kayıp senaryoları henüz test edilmedi.
