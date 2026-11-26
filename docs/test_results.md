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

Watchdog ön planda çalışırken Ctrl+C ile durduruldu. Watchdog'un PID'si 745, worker PID'leri 746 ve 747 idi. Terminal çıktısında iki worker'ın da sinyal 2 (SIGINT) ile sonlandığı görüldü.

## Henüz test edilmeyen senaryolar

Normal çıkış, beklenmedik çıkış ve üç yeniden başlatma sonrasındaki dördüncü worker kaybında yeni worker açılmaması henüz test edilmedi.
