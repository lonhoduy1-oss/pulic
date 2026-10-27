# PG Neon v0.4.0 (Experimental) — Polygun Arena 1.6100 / Build 1180

**Không phải DEB đã thử thành công trên iPhone.** Đây là mã nguồn và ARM64 object đã biên dịch, cùng workflow macOS để liên kết DYLIB và đóng DEB. Giữ DEB 0.3.2 trong `assets/` để khôi phục.

## Nguồn gốc & giới hạn

Đối chiếu trực tiếp hai tệp game do người dùng cung cấp:
- UnityFramework SHA-256: `3f0e30d1ab9bab0c559d7901d194250b798a5617af90df58243ab44c8aafbb6b`
- global-metadata.dat SHA-256: `b7b6b07db488de356290d28a10ebd5f35ffc39ee4ad75ceaf0557d4ac5956afb`
- IL2CPP metadata v39, kiến trúc ARM64, phiên bản 1.6100/b1180.

## Chức năng trong mã

- Menu PG Neon, Aim chỉ xét mục tiêu được game gắn cờ Head, ESP tia từ trên xuống + khung + HP (kế thừa v0.3.9, **chưa kiểm chứng**).
- Speed x1/x2/x3/x4/x5 bằng `UnityEngine.Time.timeScale`; không phải tốc độ nhân vật được máy chủ chấp nhận.
- GOD LOCAL: tìm **chính xác một** `CriticalStrikeFPSControlling.FPSControllerUltron` đã khởi tạo, lấy `get_Stats`; thử khôi phục `PlayerStats.Health` khi thấp hơn MaxHealth và bật cờ `PlayerStats + 0x221`. Đây chỉ là **thay đổi phía máy**; không chứng minh bất tử trong trận.
- DMG LOCAL: thử đặt `PlayerStats.DamageModifier` qua setter với các hệ số x1/x1.5/x2/x3/x5; kiểm tra đọc ngược trường `0x188`; không chứng minh sát thương thực tế của máy chủ tăng.
- Khi tìm thấy nhiều bộ điều khiển hoặc thiếu API sẽ **không sửa dữ liệu**. Khi tắt, cố khôi phục trạng thái ban đầu nếu đối tượng vẫn hợp lệ.
- Trạng thái trong menu ghi `LOCAL APPLIED`, `NO LOCAL STATS`, `CHECK FAILED`, **không bao giờ ghi `GOD SUCCESS` hoặc `SERVER DMG SUCCESS`**.

## Cài & thử

**Đây chỉ là mã nguồn**. Mã được biên dịch thành object ARM64 nhưng môi trường Linux hiện tại không có trình liên kết iOS hoạt động; do đó không có file `.deb` v0.4.0 cài sẵn.

Build bằng macOS + Xcode iOS SDK: `bash build_macos.sh`.

Hoặc đăng toàn bộ ZIP lên repo GitHub rồi bấm **Actions → Build PG Neon RootHide v0.4.0 → Run workflow**. Workflow tạo `out/PG_Neon_v040_RootHide_EXPERIMENTAL.deb` khi build thành công.

Trước khi thay bản cũ, hãy **giữ backup v0.3.2** đã xác nhận ổn. Test trên phiên bản game chính xác và tài khoản thử; bật 1 tính năng mỗi lần. Nếu phát sinh văng game, gỡ v0.4.0 và cài lại v0.3.2.

**Không có đảm bảo bất tử/buff dame thật**, vì chức năng mạng có thể có quyền quyết định riêng. Không nên dùng bản này trong trận online khi chưa kiểm chứng.

## Nội dung

- `src/PG_Neon_v040.c`: mã nguồn đầy đủ
- `src/PG_Neon_v040.arm64.o`: ARM64 Mach-O object kiểm tra bằng clang
- `build_macos.sh`: tạo DYLIB và ký bằng macOS
- `make_deb.py`: đóng DEB từ DYLIB thật, không đóng gói object giả
- `.github/workflows/build-ios.yml`: workflow macOS
- `assets/PG_Neon_v032_WORKING_BACKUP.deb`: bản cũ dự phòng
