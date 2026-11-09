package com.max2870.controller;

import com.fazecast.jSerialComm.SerialPort;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.concurrent.atomic.AtomicBoolean;

public class EspConnection {

    public static final byte HEADER_CTRL = (byte) 0xAA;
    public static final byte HEADER_DATA = (byte) 0xCC;
    public static final byte HEADER_STOP = (byte) 0xDD;
    
    public static final int CTRL_SIZE = 24;

    public interface ConnectionListener {
        void onLogMessage(String message);
        void onLockStatusChanged(boolean isLocked);
        void onError(String error);
        void onDataReceived(short[] samples);
    }

    private SerialPort serialPort;
    private OutputStream out;
    private InputStream in;
    private Thread readerThread;
    private final AtomicBoolean isRunning = new AtomicBoolean(false);
    private final ConnectionListener listener;
    
    private volatile boolean isStreaming = false;

    public EspConnection(ConnectionListener listener) {
        this.listener = listener;
    }

    // ... (методы getAvailablePorts, openPort, closePort, isOpen остаются прежними) ...

    public String[] getAvailablePorts() {
        SerialPort[] ports = SerialPort.getCommPorts();
        String[] names = new String[ports.length];
        for (int i = 0; i < ports.length; i++) names[i] = ports[i].getSystemPortName();
        return names;
    }

    public boolean openPort(String portName) {
        if (serialPort != null && serialPort.isOpen()) closePort();

        serialPort = SerialPort.getCommPort(portName);
        serialPort.setComPortParameters(115200, 8, 1, SerialPort.NO_PARITY);
        serialPort.setComPortTimeouts(SerialPort.TIMEOUT_READ_SEMI_BLOCKING, 100, 100);

        if (!serialPort.openPort()) {
            notifyError("Не удалось открыть порт: " + portName);
            return false;
        }

        try {
            out = serialPort.getOutputStream();
            in  = serialPort.getInputStream();
            startReadingLoop();
            notifyLog("Connected to " + portName);
            return true;
        } catch (Exception e) {
            notifyError("Ошибка потоков: " + e.getMessage());
            closePort();
            return false;
        }
    }

    public void closePort() {
        isRunning.set(false);
        if (readerThread != null) {
            try { readerThread.join(1000); } catch (InterruptedException ignored) {}
        }
        if (serialPort != null) serialPort.closePort();
        out = null;
        in  = null;
        isStreaming = false;
        notifyLog("Disconnected.");
    }

    public boolean isOpen() {
        return serialPort != null && serialPort.isOpen();
    }
    
    public boolean isStreamingActive() {
        return isStreaming;
    }

    // --- Отправка команд ---

    public void sendControlPacket(float freqMHz, 
                                  int cpCurrent, 
                                  int pdPolarity,      
                                  int ldfMode, 
                                  int muxOutSel, 
                                  int bandScanDiv, 
                                  int rfDivSelect, 
                                  int outputPower,
                                  int auxEnable, 
                                  int vcoPd, 
                                  int abpMode, 
                                  int chargeCancel,
                                  short tuningStep, 
                                  int ldPinMode) {
        
        if (!isOpen()) return;
 

        ByteBuffer bb = ByteBuffer.allocate(CTRL_SIZE);
        bb.order(ByteOrder.LITTLE_ENDIAN);

        bb.put(HEADER_CTRL);
        bb.putFloat(freqMHz);
        bb.put((byte) cpCurrent);
        bb.put((byte) pdPolarity);   
        bb.put((byte) ldfMode);
        bb.put((byte) muxOutSel);
        bb.put((byte) bandScanDiv);
        bb.put((byte) rfDivSelect);
        bb.put((byte) outputPower);
        bb.put((byte) auxEnable);
        bb.put((byte) vcoPd);
        bb.put((byte) abpMode);
        bb.put((byte) chargeCancel);
        bb.putShort(tuningStep);
        bb.put((byte) ldPinMode);
        bb.put((byte) 0); // Reserved

        byte[] arr = bb.array();
        long sum = 0;
        for (int i = 0; i < 20; i++) sum += (arr[i] & 0xFF);
        bb.putInt(20, (int) sum);

        try {
            out.write(arr, 0, arr.length);
            out.flush();
            notifyLog(String.format(java.util.Locale.US, ">> SEND CTRL: %.2f MHz", freqMHz));
        } catch (Exception e) {
            notifyError("Send failed: " + e.getMessage());
        }
    }

    public void startScopeStream() {
        if (!isOpen()) return;
        
        ByteBuffer bb = ByteBuffer.allocate(CTRL_SIZE);
        bb.order(ByteOrder.LITTLE_ENDIAN);
        bb.put(HEADER_CTRL);
        bb.putFloat(-1.0f); 
        for(int i=5; i<20; i++) bb.put((byte)0);
        bb.putShort((short)0); 
        
        byte[] arr = bb.array();
        long sum = 0;
        for (int i = 0; i < 20; i++) sum += (arr[i] & 0xFF);
        bb.putInt(20, (int) sum);

        try {
            out.write(arr, 0, arr.length);
            out.flush();
            isStreaming = true;
            notifyLog(">> SCOPE STREAM STARTED");
        } catch (Exception e) {
            notifyError("Start stream failed: " + e.getMessage());
        }
    }

    public void stopScopeStream() {
        if (!isOpen()) return;
        try {
            out.write(HEADER_STOP);
            out.flush();
            isStreaming = false;
            notifyLog(">> SCOPE STREAM STOPPED");
        } catch (Exception e) {
            notifyError("Stop stream failed: " + e.getMessage());
        }
    }

    // --- Прием данных ---

    private void startReadingLoop() {
        isRunning.set(true);
        readerThread = new Thread(() -> {
            byte[] buf = new byte[512];
            StringBuilder textAccumulator = new StringBuilder();
            
            byte[] dataPacketBuf = new byte[256]; 
            int packetIdx = 0;
            int expectedTotalLen = 0;

            while (isRunning.get() && serialPort != null && serialPort.isOpen()) {
                try {
                    int available = in.available();
                    if (available > 0) {
                        int read = in.read(buf, 0, Math.min(available, buf.length));
                        if (read > 0) {
                            for (int i = 0; i < read; i++) {
                                byte b = buf[i];
                                
                                if (isStreaming) {
                                    if (packetIdx == 0) {
                                        if ((b & 0xFF) == (HEADER_DATA & 0xFF)) {
                                            dataPacketBuf[packetIdx++] = b;
                                        }
                                    } else if (packetIdx == 1) {
                                        dataPacketBuf[packetIdx++] = b;
                                        int count = b & 0xFF;
                                        expectedTotalLen = 2 + (count * 2);
                                        
                                        if (expectedTotalLen > dataPacketBuf.length || count == 0) {
                                            packetIdx = 0; 
                                            continue;
                                        }
                                    } else {
                                        dataPacketBuf[packetIdx++] = b;
                                        
                                        if (packetIdx >= expectedTotalLen) {
                                            processSimpleDataProvider(dataPacketBuf, expectedTotalLen);
                                            packetIdx = 0;
                                            expectedTotalLen = 0;
                                        }
                                    }
                                } else {
                                    char c = (char) (b & 0xFF);
                                    if (c == '\n' || c == '\r') {
                                        if (textAccumulator.length() > 0) {
                                            handleLine(textAccumulator.toString().trim());
                                            textAccumulator.setLength(0);
                                        }
                                    } else if (c == '.') {
                                        handleLine(".");
                                    } else {
                                        textAccumulator.append(c);
                                    }
                                }
                            }
                        }
                    } else {
                        Thread.sleep(5); 
                    }
                } catch (Exception e) {
                    if (isRunning.get()) notifyError("Read Error: " + e.getMessage());
                    break;
                }
            }
        }, "ESP-Reader");
        readerThread.setDaemon(true);
        readerThread.start();
    }

    private void processSimpleDataProvider(byte[] raw, int totalLen) {
        if (totalLen < 3) return; 
        
        int sampleCount = (raw[1] & 0xFF);
        short[] samples = new short[sampleCount];
        
        int offset = 2; 
        
        for (int j = 0; j < sampleCount; j++) {
            if (offset + 1 < totalLen) {
                int low = raw[offset++] & 0xFF;
                int high = raw[offset++] & 0xFF;
                samples[j] = (short)((high << 8) | low);
            } else {
                break;
            }
        }
        
        if (listener != null && samples.length > 0) {
            listener.onDataReceived(samples);
        }
    }

    private void handleLine(String line) {
        if (line.isEmpty()) return;
        if (line.equals(".")) {
            notifyLock(true);
            return;
        }
        notifyLog(line);
        if (line.contains("UNLOCKED") || line.contains("WARNING")) {
            notifyLock(false);
        }
    }

    // Helpers
    private void notifyLog(String m) { if (listener != null) listener.onLogMessage(m); }
    private void notifyLock(boolean b) { if (listener != null) listener.onLockStatusChanged(b); }
    private void notifyError(String e) { if (listener != null) listener.onError(e); }
}