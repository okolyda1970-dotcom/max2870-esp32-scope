package com.max2870.controller;

import javax.swing.*;
import java.awt.*;
import java.util.Arrays;

public class OscilloscopePanel extends JPanel {

    // --- Настройки отображения ---
    private double timePerDiv = 50.0;   // Время на деление (условные единицы)
    private double voltPerDiv = 50.0;  // Напряжение на деление (в LSB АЦП)
    private int posX = 0;              // Смещение по X (в точках буфера)
    private int posY = 0;              // Смещение по Y (в пикселях)
    private int triggerLevel = 2048;   // Уровень триггера
    private boolean isPaused = false;  
    private boolean autoScaleEnabled = false; // По умолчанию выключено для ручного управления

    // --- Константы сетки ---
    private static final int GRID_COLS = 10; 
    private static final int GRID_ROWS = 8;  

    // --- Буфер данных ---
    private short[] dataBuffer;        
    private int bufferSize = 4096;     
    private int writeIndex = 0;        
    private int validDataCount = 0;    

    private Timer repaintTimer;

    public OscilloscopePanel() {
        setBackground(Color.BLACK);
        setMinimumSize(new Dimension(400, 250));
        
        dataBuffer = new short[bufferSize];
        Arrays.fill(dataBuffer, (short) 2048); 
        
        repaintTimer = new Timer(33, e -> {
            if (!isPaused) {
                if (autoScaleEnabled && validDataCount > 0) {
                    performAutoScale();
                }
                repaint();
            }
        });
        repaintTimer.start();
    }

    public void updateData(short[] newData) {
        if (isPaused) return; 

        synchronized (dataBuffer) {
            for (short sample : newData) {
                dataBuffer[writeIndex] = sample;
                writeIndex = (writeIndex + 1) % bufferSize;
                
                if (validDataCount < bufferSize) {
                    validDataCount++;
                }
            }
        }
    }

    private void performAutoScale() {
        if (validDataCount == 0) return;

        int minVal = 4095;
        int maxVal = 0;
        
        int scanPoints = Math.min(validDataCount, 500);
        int startIdx = (writeIndex - scanPoints + bufferSize) % bufferSize;
        
        for (int i = 0; i < scanPoints; i++) {
            int idx = (startIdx + i) % bufferSize;
            int val = dataBuffer[idx] & 0xFFFF;
            if (val < minVal) minVal = val;
            if (val > maxVal) maxVal = val;
        }

        int range = maxVal - minVal;
        if (range < 10) range = 10; 
        
        // Авто-масштаб старается вписать сигнал в 6 клеток из 8
        voltPerDiv = range / 6.0; 
        if (voltPerDiv < 10) voltPerDiv = 100; 
        
        triggerLevel = (minVal + maxVal) / 2;
        posY = 0;
        posX = 0;
    }

    @Override
    protected void paintComponent(Graphics g) {
        super.paintComponent(g);
        Graphics2D g2d = (Graphics2D) g;
        g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);

        int width = getWidth();
        int height = getHeight();
        int centerX = width / 2;
        int centerY = height / 2;

        // 1. Рисуем сетку
        drawGrid(g2d, width, height, centerX, centerY);

        if (validDataCount == 0) {
            g2d.setColor(Color.GRAY);
            g2d.drawString("No Signal Data", centerX - 40, centerY);
            return;
        }

        // 2. Поиск индекса триггера (для синхронизации по X)
        int triggerIndex = findTriggerIndex();
        if (triggerIndex == -1) {
            // Если триггер не найден, берем последнюю точку как начало развертки
            triggerIndex = (writeIndex - 1 + bufferSize) % bufferSize;
        }

        g2d.setColor(Color.GREEN);
        g2d.setStroke(new BasicStroke(1.5f));
        
        Polygon wavePoly = new Polygon();
        
        // --- ГОРИЗОНТАЛЬНЫЙ МАСШТАБ (Time/Div) ---
        // Логика: Ползунок вправо (увеличение timePerDiv) -> Сигнал РАСТЯГИВАЕТСЯ (меньше точек на экране).
        
        int maxSamplesOnScreen = 800; // Максимум точек при сильном сжатии
        int minSamplesOnScreen = 20;  // Минимум точек при сильном растяжении
        
        // Нормализация времени (предполагаем диапазон ползунка 1..100)
        double normTime = timePerDiv / 100.0;
        if (normTime < 0) normTime = 0;
        if (normTime > 1) normTime = 1;
        
        // Интерполяция: 
        // При timePerDiv=1 (влево) -> samplesToShow близко к max (сжато)
        // При timePerDiv=100 (вправо) -> samplesToShow близко к min (растянуто)
        int samplesToShow = (int)(maxSamplesOnScreen - (normTime * (maxSamplesOnScreen - minSamplesOnScreen)));
        
        // Ограничения
        if (samplesToShow > validDataCount) samplesToShow = validDataCount;
        if (samplesToShow < 1) samplesToShow = 1;

        // Начальная позиция в буфере (центрируем триггер)
        int startXInBuffer = (triggerIndex - samplesToShow / 2 + posX + bufferSize) % bufferSize;

        // --- ВЕРТИКАЛЬНЫЙ МАСШТАБ (Volt/Div) + POS Y ---
        
        double cellHeightPx = height / (double) GRID_ROWS;
        
        // 1. Нормализация значения voltPerDiv (предполагаем диапазон 10..1000)
        double minRange = 10.0;  
        double maxRange = 1000.0; 
        double t = (voltPerDiv - minRange) / (maxRange - minRange);
        if (t < 0) t = 0;
        if (t > 1) t = 1;
        
        // 2. Степенная функция (вы просили powerFactor = 0.6)
        // При 0.6: малые значения voltPerDiv дают БОЛЬШУЮ амплитуду (обратная логика), 
        // большие значения дают МАЛЕНЬКУЮ амплитуду.
        // Если хотите стандартную логику (вправо = больше), используйте степень > 1.0 (например 1.5).
        // Но раз вы выбрали 0.6, оставляем её.
        double powerFactor = 0.6; 
        
        double currentScaleRatio = Math.pow(t, powerFactor);
        
        // Базовый масштаб. Подбираем так, чтобы при t=0 (минимум ползунка) волна была максимальной.
        // targetMaxScale - это сколько пикселей на 1 LSB при полной растяжке.
        // Пусть полная растяжка занимает 90% высоты экрана.
        double targetMaxPixelsPerLSB = (height * 0.9) / 4096.0; 
        
        // Итоговый масштаб зависит от степени
        // При t=0 -> ratio=0 -> scale=0? Нет, нужно добавить базовое значение.
        // Лучше использовать формулу: Scale = MinScale + (MaxScale-MinScale) * Ratio
        
        double minAllowedScale = 0.01; // Минимальный масштаб (волна плоская линия)
        double maxAllowedScale = targetMaxPixelsPerLSB; // Максимальный масштаб
        
        double pixelsPerLSB = minAllowedScale + (maxAllowedScale - minAllowedScale) * currentScaleRatio;
        
        // ВАЖНО: Для posY мы НЕ делаем clamp (ограничение) на этапе расчета координат точки.
        // Мы просто считаем реальную позицию. Swing Graphics2D автоматически обрезает то, что вне bounds.
        
        // Отрисовка волны
        for (int i = 0; i < samplesToShow; i++) {
            int bufIdx = (startXInBuffer + i) % bufferSize;
            int rawVal = dataBuffer[bufIdx] & 0xFFFF;
            
            int deltaFromMid = rawVal - 2048;
            
            // Расчет Y координаты
            // Экранная ось Y растет вниз (0 сверху).
            // deltaFromMid * pixelsPerLSB дает смещение относительно центра.
            // MINUS posY означает: если posY > 0, то screenY уменьшается -> точка идет ВВЕРХ.
            
            int screenY = (int)(centerY - (deltaFromMid * pixelsPerLSB)) - posY;
            
            // УБИРАЕМ CLAMP HERE! 
            // Пусть точка уходит за пределы -1000 или +1000. 
            // Полигон все равно отрисуется корректно, а линии, идущие "в бесконечность", будут видны только до края окна.
            
            int screenX = (int)((i * width) / samplesToShow);

            wavePoly.addPoint(screenX, screenY);
        }

        if (wavePoly.npoints > 1) {
            g2d.drawPolyline(wavePoly.xpoints, wavePoly.ypoints, wavePoly.npoints);
        }
        
        drawMarkers(g2d, width, height, centerX, centerY);
    }

    private int findTriggerIndex() {
        if (validDataCount < 10) return -1;
        
        int searchLimit = Math.min(validDataCount, 1000); 
        int startIndex = (writeIndex - searchLimit + bufferSize) % bufferSize;
        
        for (int i = 0; i < searchLimit - 1; i++) {
            int currIdx = (startIndex + i) % bufferSize;
            int nextIdx = (currIdx + 1) % bufferSize;
            
            int currVal = dataBuffer[currIdx] & 0xFFFF;
            int nextVal = dataBuffer[nextIdx] & 0xFFFF;
            
            // Нарастающий фронт
            if (currVal <= triggerLevel && nextVal > triggerLevel) {
                return currIdx;
            }
        }
        return -1; 
    }

    private void drawGrid(Graphics2D g2d, int w, int h, int cx, int cy) {
        g2d.setColor(new Color(40, 40, 40));
        g2d.setStroke(new BasicStroke(1));
        
        // Вертикальные линии (Time divisions)
        int colWidth = w / GRID_COLS;
        for (int i = 1; i < GRID_COLS; i++) {
            int x = i * colWidth;
            g2d.drawLine(x, 0, x, h);
        }
        
        // Горизонтальные линии (Volt divisions)
        int rowHeight = h / GRID_ROWS;
        for (int i = 1; i < GRID_ROWS; i++) {
            int y = i * rowHeight;
            g2d.drawLine(0, y, w, y);
        }
        
        // Центральные оси
        g2d.setColor(new Color(80, 80, 80));
        g2d.drawLine(cx, 0, cx, h);
        g2d.drawLine(0, cy, w, cy);
    }
    
    private void drawMarkers(Graphics2D g2d, int w, int h, int cx, int cy) {
        g2d.setColor(Color.YELLOW);
        g2d.setFont(new Font("SansSerif", Font.BOLD, 12));
        
        String info = String.format("T/DIV: %.1f | V/DIV: %.0f LSB | TRIG: %d | PAUSE: %s", 
                                    timePerDiv, voltPerDiv, triggerLevel, isPaused ? "YES" : "NO");
        g2d.drawString(info, 10, 20);
        
        // Линия уровня триггера (пунктир)
        g2d.setColor(new Color(255, 100, 100, 150)); 
        g2d.setStroke(new BasicStroke(1, BasicStroke.CAP_BUTT, BasicStroke.JOIN_BEVEL, 0, new float[]{5}, 0));
        
        // Рассчитываем Y позицию триггера на экране
        double cellHeightPx = h / (double) GRID_ROWS;
        double pixelsPerLSB = cellHeightPx / voltPerDiv;
        if (pixelsPerLSB <= 0 || Double.isNaN(pixelsPerLSB)) pixelsPerLSB = 0.01;
        
        int trigDelta = triggerLevel - 2048;
        int trigY = (int)(cy - (trigDelta * pixelsPerLSB)) + posY;
        
        g2d.drawLine(0, trigY, w, trigY);
    }

    // Геттеры и сеттеры
    public void setTimePerDiv(double t) { this.timePerDiv = t; }
    public void setVoltPerDiv(double v) { this.voltPerDiv = v; }
    public void setPosX(int p) { this.posX = p; }
    public void setPosY(int p) { this.posY = p; }
    public void setTriggerLevel(int lvl) { this.triggerLevel = lvl; }
    public void setPaused(boolean pause) { this.isPaused = pause; }
    public boolean isPaused() { return isPaused; }
    public void setAutoScale(boolean enable) { this.autoScaleEnabled = enable; }
}