# Test Sonuçları

Tarih: 26 Kasım 2026
Ortam: Linux (Ubuntu/WSL)

## Otomatik test betiği

Çalıştırılan komut: ./tests/test_scenarios.sh

Sonuçlar:

- İki worker başlatıldı. İlk çalıştırmadaki PID'ler: 706 ve 707.
- SIGUSR1 durum raporunda iki worker'ın PID'leri loglandı.
- Yalnızca watchdog'a SIGTERM gönderildi; watchdog kapandı ve worker süreçleri toplandı.
- İkinci çalıştırmadaki worker PID'leri: 726 ve 727.
- Yalnızca watchdog'a SIGINT gönderildi; watchdog kapandı ve worker süreçleri toplandı.
- Test betiği mevcut senaryoların tamamını başarılı bildirdi.

## Klavyeden Ctrl+C denemesi

İlk denemede worker’lar SIGINT (sinyal 2) ile sonlandı. Ana entegrasyon sonrası Ctrl+C tekrar test edildi: watchdog PID 1537 kapandı; worker PID’leri 1538 ve 1539 SIGTERM (sinyal 15) ile sonlandı.


## Ana entegrasyon sonrası gelişmiş testler

Çalıştırılan komut: `./tests/test_advanced_scenarios.sh`

- Kod 0 ile çıkan iki worker restart edilmedi.
- Kod 7 ile çıkan worker’lar üçer kez restart edildi. Her birinin dördüncü kaybında `restart_limit` kaydı oluştu; yeni worker açılmadı.
- 5 saniyelik periyodik kontrolde iki worker da `stopped`, restart sayısı `3/3` olarak loglandı.
- SIGKILL ile beklenmedik sonlanan worker için `worker_signal`, `restart_attempt` ve `worker_restarted` kayıtları oluştu.
- Betik `Tüm gelişmiş test senaryoları başarılı.` çıktısını verdi.
