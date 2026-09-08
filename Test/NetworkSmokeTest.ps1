# ─────────────────────────────────────────────────────────────────────────────
# Network 계층 수동 smoke test 스크립트.
#
# 사용법:
#   1) 서버를 "서버 모드"로 먼저 실행해 둔다 (둘 중 하나).
#      - Visual Studio: 프로젝트 속성 > 디버깅 > 명령 인수에 "--server 9000" 입력 후 Ctrl+F5
#      - 또는 빌드된 exe를 콘솔에서 직접:  GameEventSystem.exe --server 9000
#   2) 새 PowerShell 창을 열고 이 스크립트를 실행한다.
#        powershell -ExecutionPolicy Bypass -File Test\NetworkSmokeTest.ps1
#
# 하는 일:
#   CS_LOGIN(uid=12345) -> SC_LOGIN_ACK 수신 확인
#   CS_EVENT_INFO_REQ   -> SC_EVENT_INFO_ACK(또는 SC_ERROR) 수신 확인
#
# Network/PacketDefs.h의 오퍼코드/구조체와 바이트 단위로 맞춰져 있다.
# ─────────────────────────────────────────────────────────────────────────────

param(
    [string]$ServerHost = "127.0.0.1",
    [int]$Port = 9000
)

$ErrorActionPreference = "Stop"

Write-Host "connecting to $ServerHost`:$Port ..."
$client = New-Object System.Net.Sockets.TcpClient($ServerHost, $Port)
$stream = $client.GetStream()

function Send-Packet {
    param([uint16]$Opcode, [byte[]]$Payload = @())

    $totalSize = 4 + $Payload.Length
    $buf = New-Object byte[] $totalSize
    [System.BitConverter]::GetBytes([uint16]$totalSize).CopyTo($buf, 0)
    [System.BitConverter]::GetBytes([uint16]$Opcode).CopyTo($buf, 2)
    if ($Payload.Length -gt 0) { $Payload.CopyTo($buf, 4) }

    $stream.Write($buf, 0, $buf.Length)
    $stream.Flush()
}

function Read-Packet {
    $header = New-Object byte[] 4
    $read = 0
    while ($read -lt 4) {
        $n = $stream.Read($header, $read, 4 - $read)
        if ($n -le 0) { throw "connection closed while reading header" }
        $read += $n
    }

    $size   = [System.BitConverter]::ToUInt16($header, 0)
    $opcode = [System.BitConverter]::ToUInt16($header, 2)
    $bodyLen = $size - 4

    $body = New-Object byte[] ([Math]::Max($bodyLen, 0))
    $read = 0
    while ($read -lt $bodyLen) {
        $n = $stream.Read($body, $read, $bodyLen - $read)
        if ($n -le 0) { throw "connection closed while reading body" }
        $read += $n
    }

    return [PSCustomObject]@{ Opcode = $opcode; Body = $body }
}

# ── 1) CS_LOGIN (opcode 1001), payload = uid (uint64 LE, 8바이트) ──────────────
$uid = 12345
$uidBytes = [System.BitConverter]::GetBytes([uint64]$uid)
Write-Host "`n[1] CS_LOGIN 전송 (uid=$uid)"
Send-Packet -Opcode 1001 -Payload $uidBytes

# 서버 틱(100ms) + Mock DB 지연(20ms)을 감안해 넉넉히 대기
Start-Sleep -Milliseconds 400
$loginAck = Read-Packet
$success = if ($loginAck.Body.Length -gt 0) { $loginAck.Body[0] } else { -1 }
Write-Host ("    SC_LOGIN_ACK 수신: opcode={0} success={1}" -f $loginAck.Opcode, $success)

if ($loginAck.Opcode -ne 1002 -or $success -ne 1) {
    Write-Warning "로그인 응답이 예상과 다릅니다. opcode는 1002(SC_LOGIN_ACK), success는 1이어야 합니다."
}

# ── 2) CS_EVENT_INFO_REQ (opcode 2001), payload 없음 ──────────────────────────
Write-Host "`n[2] CS_EVENT_INFO_REQ 전송"
Send-Packet -Opcode 2001

Start-Sleep -Milliseconds 200
$infoAck = Read-Packet
Write-Host ("    응답 수신: opcode={0} (2002=SC_EVENT_INFO_ACK, 9001=SC_ERROR)" -f $infoAck.Opcode)

$client.Close()
Write-Host "`n완료."
