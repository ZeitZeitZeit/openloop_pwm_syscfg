# SOPWM — Luồng dựng lịch chuyển mạch (đã kiểm tra theo code)

Nguồn: `pwm/sopwm/sopwm.c` (`SOPWM_BuildSchedule`, `sopwm_build_phase`), `sopwm.h`, `main.c`.
Ví dụ minh hoạ dùng **N = 7** -> **M = 3** góc cơ sở -> **4·M = 12** góc cả chu kỳ.

> Lưu ý: `GetAngles()` KHÔNG ghi thẳng ra thanh ghi CMP. Góc phải đi qua Build Schedule ->
> bảng `sopwm_sched` (INACTIVE buffer) -> DMA mới tới được CMPA/CMPB của ePWM.

---

## Luồng tổng quát

```mermaid
flowchart TB
    LUT["LUTs (vd SOPWM_LUT_N7[100][3])<br/>100 mức m × M góc, đơn vị độ"]
    --> BUILD["SOPWM_BuildSchedule(m, N, TBPRD)"]
    BUILD --> S1
    S1["1) GetAngles(m, N)<br/>-> M góc cơ sở (Q1, 0-90°)<br/>vd N=7 -> 3 góc"]
    --> S2["2) Mở rộng quarter-wave<br/>-> 4·M góc phủ 0-360°<br/>vd 12 góc"]
    --> S3["3) Dịch pha cho B (+120°) và C (+240°)<br/>rồi wrap về [0, 360°)"]
    --> S4["4) Gán mỗi góc vào 1 khe sóng răng cưa<br/>slot = góc / 7.2°<br/>local = góc − slot×7.2°<br/>counts = (local / 7.2°) × TBPRD"]
    --> S5["5) Ghi {cmpa, cmpb} vào INACTIVE buffer<br/>sopwm_sched[phase][next_buf][slot]"]
    --> DMA["DMA nạp CMPA/CMPB vào ePWM tại mỗi ZERO"]
```

---

## Chi tiết bước 2 — Mở rộng quarter-wave (M=3 -> 12 góc)

```mermaid
flowchart LR
    BASE["3 góc cơ sở: a1, a2, a3"]
    BASE --> Q1["Q1: a1, a2, a3"]
    BASE --> Q2["Q2: 180−a3, 180−a2, 180−a1"]
    BASE --> Q3["Q3: 180+a1, 180+a2, 180+a3"]
    BASE --> Q4["Q4: 360−a3, 360−a2, 360−a1"]
    Q1 --> ALL["12 góc cả chu kỳ"]
    Q2 --> ALL
    Q3 --> ALL
    Q4 --> ALL
```

---

## Chi tiết bước 4 + 5 — Gán khe & ghi CMP (sopwm_build_phase)

```mermaid
flowchart TB
    INIT["Khởi tạo cả 50 khe:<br/>cmpa = cmpb = CMP_OFF (0xFFFF)"]
    --> LOOP["Lặp qua từng góc của pha (4·M góc)"]
    LOOP --> SLOT["slot = góc / 7.2°  (kẹp < 50)"]
    SLOT --> CNT["local = góc − slot×7.2°<br/>counts = (local / 7.2°) × TBPRD  (kẹp < TBPRD)"]
    CNT --> CHK{"Khe đã có mấy điểm chuyển?"}
    CHK -- "0 (đang OFF)" --> A["cmpa = counts"]
    CHK -- "1 (đã có cmpa)" --> B["cmpb = counts<br/>giữ cmpa < cmpb (đổi chỗ nếu cần)"]
    CHK -- "2 (đã đủ)" --> D["bỏ qua góc này<br/>(chỉ xảy ra khi m ≥ 0.97)"]
    A --> NEXT
    B --> NEXT
    D --> NEXT["góc tiếp theo"]
    NEXT --> LOOP
    LOOP -- "hết góc" --> HW["Đối xứng nửa chu kỳ:<br/>mid_slot = (offset+180°)/7.2°<br/>ép counts ≈ TBPRD/2 vào cmpa/cmpb còn trống"]
    HW --> DONE["sopwm_sched[phase][next_buf][slot] đã sẵn sàng"]
```

---

## Bối cảnh — lịch nằm ở đâu và ai đọc

```mermaid
flowchart LR
    BS["BuildSchedule ghi vào<br/>buffer KHÔNG hoạt động (next_buf)"]
    --> COMMIT["CommitSchedule: đánh dấu 'sẵn sàng'"]
    --> SWAP["schedISR: hoán đổi active/next<br/>tại ranh giới chu kỳ (khe 50)"]
    --> DMARD["DMA đọc buffer ACTIVE<br/>mỗi khe nạp 1 cặp {cmpa, cmpb}"]
    --> CMP["CMPA/CMPB của EPWM1/4/7<br/>-> Action Qualifier lật chân -> Dead-band -> GPIO"]
```

---

### Ghi chú nhanh
- `sopwm_sched` có dạng `[3 pha][2 buffer][50 khe]`, mỗi phần tử = `{cmpa, cmpb}`.
- `CMP_OFF = 0xFFFF` > `TBPRD` nên khe không có điểm chuyển sẽ không bao giờ khớp -> không lật.
- 50 khe = 50 chu kỳ sóng mang (50 kHz) trong 1 chu kỳ cơ bản (1 kHz); mỗi khe = 7.2°.
- `TBPRD` trong build = 2000 (`main.c`), SysConfig đặt period = 1999.
