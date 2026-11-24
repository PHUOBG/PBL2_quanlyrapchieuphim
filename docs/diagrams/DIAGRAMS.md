# Sơ đồ Kiến trúc & Thiết kế CiNeXiNe

## 1. Sơ đồ phân tầng tổng thể
```
┌────────────────────────────────────────────────────────┐
│                   Presentation Layer                   │
│   (Screens, Admin Panels, Customer Panels, Components) │
└───────────┬────────────────────────────────┬───────────┘
            │                                │
            ▼                                ▼
┌────────────────────────┐      ┌────────────────────────┐
│     Services Layer     │      │    Exporters Module    │
│  (Auth, Booking, ...)  │      │(Template Method Pattern│
└───────────┬────────────┘      └────────────┬───────────┘
            │                                │
            ▼                                │
┌────────────────────────┐                   │
│   Repositories Layer   │                   │
│ (User, Movie, Ticket)  │                   │
└───────────┬────────────┘                   │
            │                                │
            ▼                                ▼
┌────────────────────────┐      ┌────────────────────────┐
│      data/ (.txt)      │      │     exports/ (.csv)    │
│    (Persistent Data)   │      │   (Generated Reports)  │
└────────────────────────┘      └────────────────────────┘
```

## 2. Sơ đồ Template Method Pattern (CSV Exporter)
```
                       ┌─────────────────────────┐
                       │       <<abstract>>      │
                       │       CsvExporter       │
                       ├─────────────────────────┤
                       │ + exportToCsv()         │
                       │ # writeReportHeader()*  │
                       │ # writeData()*          │
                       │ # writeSummary()        │
                       └────────────┬────────────┘
                                    │
       ┌──────────────┬─────────────┼──────────────┬──────────────┐
       │              │             │              │              │
┌──────┴───────┐┌─────┴───────┐┌────┴────────┐┌────┴────────┐┌────┴────────┐
│RevenueReport ││OverviewRep. ││CustomerRep. ││ComboReport  ││VoucherRep.  │
│Exporter      ││Exporter     ││Exporter     ││Exporter     ││Exporter     │
└──────────────┘└─────────────┘└─────────────┘└─────────────┘└─────────────┘
```

