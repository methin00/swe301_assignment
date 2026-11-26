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

Ön planda çalışan watchdog’u `Ctrl+C` ile de durdurabilirsin. Worker’lar terminalden gelen SIGINT’i yok sayar. Watchdog kapanışı başlatır, worker’lara SIGTERM gönderir ve sonlanmalarını bekler.

## Testleri çalıştırma

```bash
./tests/test_scenarios.sh
```

Test betiği iki worker’ın başlamasını, SIGUSR1 durum raporunu ve watchdog’a gönderilen SIGTERM/SIGINT sonrasında worker’ların kapanmasını kontrol eder. Örnek log `docs/sample_watchdog.log` dosyasındadır.

Ek senaryolar ve restart sınırı için:

```bash
./tests/test_advanced_scenarios.sh
```

Bu betik kod 0 ile normal çıkışı, kod 7 ile hatalı çıkışı, her worker için üç restart sınırını, SIGKILL sonrası restartı ve beş saniyelik periyodik kontrolü sınar. Gerçek Ctrl+C denemesinde watchdog kapandı ve iki worker SIGTERM (sinyal 15) ile sonlandı.
