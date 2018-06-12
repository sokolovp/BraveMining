// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.selection;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.ArgumentMatchers.isA;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.when;

import android.content.ContentResolver;
import android.content.Context;
import android.content.pm.PackageManager;
import android.provider.Settings;
import android.view.ActionMode;
import android.view.View;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.InOrder;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowLog;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.content.browser.RenderCoordinates;
import org.chromium.content.browser.webcontents.WebContentsImpl;
import org.chromium.content_public.browser.SelectionClient;
import org.chromium.content_public.browser.SelectionMetricsLogger;
import org.chromium.content_public.browser.SelectionPopupController;
import org.chromium.ui.base.MenuSourceType;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.touch_selection.SelectionEventType;

/**
 * Unit tests for {@link SelectionPopupController}.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class SelectionPopupControllerTest {
    private SelectionPopupControllerImpl mController;
    private Context mContext;
    private WindowAndroid mWindowAndroid;
    private WebContentsImpl mWebContents;
    private View mView;
    private ActionMode mActionMode;
    private PackageManager mPackageManager;
    private SmartSelectionMetricsLogger mLogger;
    private RenderCoordinates mRenderCoordinates;
    private ContentResolver mContentResolver;

    private static final String MOUNTAIN_FULL = "585 Franklin Street, Mountain View, CA 94041";
    private static final String MOUNTAIN = "Mountain";
    private static final String AMPHITHEATRE_FULL = "1600 Amphitheatre Parkway";
    private static final String AMPHITHEATRE = "Amphitheatre";

    private static class TestSelectionClient implements SelectionClient {
        private SelectionClient.Result mResult;
        private SelectionClient.ResultCallback mResultCallback;
        private SmartSelectionMetricsLogger mLogger;

        @Override
        public void onSelectionChanged(String selection) {}

        @Override
        public void onSelectionEvent(int eventType, float posXPix, float poxYPix) {}

        @Override
        public void showUnhandledTapUIIfNeeded(int x, int y) {}

        @Override
        public void selectWordAroundCaretAck(boolean didSelect, int startAdjust, int endAdjust) {}

        @Override
        public boolean requestSelectionPopupUpdates(boolean shouldSuggest) {
            mResultCallback.onClassified(mResult);
            return true;
        }

        @Override
        public void cancelAllRequests() {}

        @Override
        public SelectionMetricsLogger getSelectionMetricsLogger() {
            return mLogger;
        }

        public void setResult(SelectionClient.Result result) {
            mResult = result;
        }

        public void setResultCallback(SelectionClient.ResultCallback callback) {
            mResultCallback = callback;
        }

        public void setLogger(SmartSelectionMetricsLogger logger) {
            mLogger = logger;
        }
    }

    private static class SelectionClientOnlyReturnTrue extends TestSelectionClient {
        @Override
        public boolean requestSelectionPopupUpdates(boolean shouldSuggest) {
            return true;
        }
    }

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        ShadowLog.stream = System.out;

        mContext = Mockito.mock(Context.class);
        mWindowAndroid = Mockito.mock(WindowAndroid.class);
        mWebContents = Mockito.mock(WebContentsImpl.class);
        mView = Mockito.mock(View.class);
        mActionMode = Mockito.mock(ActionMode.class);
        mPackageManager = Mockito.mock(PackageManager.class);
        mRenderCoordinates = Mockito.mock(RenderCoordinates.class);
        mLogger = Mockito.mock(SmartSelectionMetricsLogger.class);

        mContentResolver = RuntimeEnvironment.application.getContentResolver();
        // To let isDeviceProvisioned() call in showSelectionMenu() return true.
        Settings.System.putInt(mContentResolver, Settings.Global.DEVICE_PROVISIONED, 1);

        when(mContext.getPackageManager()).thenReturn(mPackageManager);
        when(mContext.getContentResolver()).thenReturn(mContentResolver);
        when(mWebContents.getRenderCoordinates()).thenReturn(mRenderCoordinates);
        when(mRenderCoordinates.getDeviceScaleFactor()).thenReturn(1.f);

        mController = SelectionPopupControllerImpl.createForTesting(
                mContext, mWindowAndroid, mWebContents, mView);
    }

    @Test
    @Feature({"TextInput", "SmartSelection"})
    public void testSmartSelectionAdjustSelectionRange() {
        InOrder order = inOrder(mWebContents, mView);
        SelectionClient.Result result = resultForAmphitheatre();

        // Setup SelectionClient for SelectionPopupController.
        TestSelectionClient client = new TestSelectionClient();
        client.setResult(result);
        client.setResultCallback(mController.getResultCallback());
        mController.setSelectionClient(client);

        // Long press triggered showSelectionMenu() call.
        mController.showSelectionMenu(0, 0, 0, 0, 0, /* isEditable = */ true,
                /* isPasswordType = */ false, AMPHITHEATRE, /* selectionOffset = */ 5,
                /* canSelectAll = */ true,
                /* canRichlyEdit = */ true, /* shouldSuggest = */ true,
                MenuSourceType.MENU_SOURCE_LONG_PRESS);

        // adjustSelectionByCharacterOffset() should be called.
        order.verify(mWebContents)
                .adjustSelectionByCharacterOffset(result.startAdjust, result.endAdjust, true);
        assertFalse(mController.isActionModeValid());

        when(mView.startActionMode(any(FloatingActionModeCallback.class), anyInt()))
                .thenReturn(mActionMode);

        // Call showSelectionMenu again, which is adjustSelectionByCharacterOffset triggered.
        mController.showSelectionMenu(0, 0, 0, 0, 0, /* isEditable = */ true,
                /* isPasswordType = */ false, AMPHITHEATRE_FULL, /* selectionOffset = */ 0,
                /* canSelectAll = */ true,
                /* canRichlyEdit = */ true, /* shouldSuggest = */ true,
                MenuSourceType.MENU_SOURCE_ADJUST_SELECTION);

        order.verify(mView).startActionMode(
                isA(FloatingActionModeCallback.class), eq(ActionMode.TYPE_FLOATING));

        SelectionClient.Result returnResult = mController.getClassificationResult();
        assertEquals(-5, returnResult.startAdjust);
        assertEquals(8, returnResult.endAdjust);
        assertEquals("Maps", returnResult.label);

        assertTrue(mController.isActionModeValid());
    }

    @Test
    @Feature({"TextInput", "SmartSelection"})
    public void testSmartSelectionAnotherLongPressAfterAdjustment() {
        InOrder order = inOrder(mWebContents, mView);
        SelectionClient.Result result = resultForAmphitheatre();
        SelectionClient.Result newResult = resultForMountain();

        // Set SelectionClient for SelectionPopupController.
        TestSelectionClient client = new TestSelectionClient();
        client.setResult(result);
        client.setResultCallback(mController.getResultCallback());
        mController.setSelectionClient(client);

        // Long press triggered showSelectionMenu() call.
        mController.showSelectionMenu(0, 0, 0, 0, 0, /* isEditable = */ true,
                /* isPasswordType = */ false, AMPHITHEATRE, /* selectionOffset = */ 5,
                /* canSelectAll = */ true,
                /* canRichlyEdit = */ true, /* shouldSuggest = */ true,
                MenuSourceType.MENU_SOURCE_LONG_PRESS);

        // adjustSelectionByCharacterOffset() should be called.
        order.verify(mWebContents)
                .adjustSelectionByCharacterOffset(result.startAdjust, result.endAdjust, true);
        assertFalse(mController.isActionModeValid());

        // Another long press triggered showSelectionMenu() call.
        client.setResult(newResult);
        mController.showSelectionMenu(0, 0, 0, 0, 0, /* isEditable = */ true,
                /* isPasswordType = */ false, MOUNTAIN, /* selectionOffset = */ 21,
                /* canSelectAll = */ true,
                /* canRichlyEdit = */ true, /* shouldSuggest = */ true,
                MenuSourceType.MENU_SOURCE_LONG_PRESS);
        order.verify(mWebContents)
                .adjustSelectionByCharacterOffset(newResult.startAdjust, newResult.endAdjust, true);
        assertFalse(mController.isActionModeValid());

        when(mView.startActionMode(any(FloatingActionModeCallback.class), anyInt()))
                .thenReturn(mActionMode);

        // First adjustSelectionByCharacterOffset() triggered.
        mController.showSelectionMenu(0, 0, 0, 0, 0, /* isEditable = */ true,
                /* isPasswordType = */ false, AMPHITHEATRE_FULL, /* selectionOffset = */ 0,
                /* canSelectAll = */ true,
                /* canRichlyEdit = */ true, /* shouldSuggest = */ true,
                MenuSourceType.MENU_SOURCE_ADJUST_SELECTION);

        SelectionClient.Result returnResult = mController.getClassificationResult();
        assertEquals(-21, returnResult.startAdjust);
        assertEquals(15, returnResult.endAdjust);
        assertEquals("Maps", returnResult.label);

        // Second adjustSelectionByCharacterOffset() triggered.
        mController.showSelectionMenu(0, 0, 0, 0, 0, /* isEditable = */ true,
                /* isPasswordType = */ false, MOUNTAIN_FULL, /* selectionOffset = */ 0,
                /* canSelectAll = */ true,
                /* canRichlyEdit = */ true, /* shouldSuggest = */ true,
                MenuSourceType.MENU_SOURCE_ADJUST_SELECTION);

        order.verify(mView).startActionMode(
                isA(FloatingActionModeCallback.class), eq(ActionMode.TYPE_FLOATING));
        assertTrue(mController.isActionModeValid());
    }

    @Test
    @Feature({"TextInput", "SmartSelection"})
    public void testSmartSelectionAnotherLongPressBeforeAdjustment() {
        InOrder order = inOrder(mWebContents, mView);
        SelectionClient.Result result = resultForAmphitheatre();
        SelectionClient.Result newResult = resultForMountain();

        // This client won't call SmartSelectionCallback.
        TestSelectionClient client = new SelectionClientOnlyReturnTrue();
        mController.setSelectionClient(client);

        // Long press triggered showSelectionMenu() call.
        mController.showSelectionMenu(0, 0, 0, 0, 0, /* isEditable = */ true,
                /* isPasswordType = */ false, AMPHITHEATRE, /* selectionOffset = */ 5,
                /* canSelectAll = */ true,
                /* canRichlyEdit = */ true, /* shouldSuggest = */ true,
                MenuSourceType.MENU_SOURCE_LONG_PRESS);

        // Another long press triggered showSelectionMenu() call.
        mController.showSelectionMenu(0, 0, 0, 0, 0, /* isEditable = */ true,
                /* isPasswordType = */ false, MOUNTAIN, /* selectionOffset = */ 21,
                /* canSelectAll = */ true,
                /* canRichlyEdit = */ true, /* shouldSuggest = */ true,
                MenuSourceType.MENU_SOURCE_LONG_PRESS);

        // Then we done with the first classification.
        mController.getResultCallback().onClassified(result);

        // Followed by the second classifaction.
        mController.getResultCallback().onClassified(newResult);

        // adjustSelectionByCharacterOffset() should be called.
        order.verify(mWebContents)
                .adjustSelectionByCharacterOffset(result.startAdjust, result.endAdjust, true);
        order.verify(mWebContents)
                .adjustSelectionByCharacterOffset(newResult.startAdjust, newResult.endAdjust, true);
        assertFalse(mController.isActionModeValid());

        when(mView.startActionMode(any(FloatingActionModeCallback.class), anyInt()))
                .thenReturn(mActionMode);

        // First adjustSelectionByCharacterOffset() triggered.
        mController.showSelectionMenu(0, 0, 0, 0, 0, /* isEditable = */ true,
                /* isPasswordType = */ false, AMPHITHEATRE_FULL, /* selectionOffset = */ 0,
                /* canSelectAll = */ true,
                /* canRichlyEdit = */ true, /* shouldSuggest = */ true,
                MenuSourceType.MENU_SOURCE_ADJUST_SELECTION);

        SelectionClient.Result returnResult = mController.getClassificationResult();
        assertEquals(-21, returnResult.startAdjust);
        assertEquals(15, returnResult.endAdjust);
        assertEquals("Maps", returnResult.label);

        // Second adjustSelectionByCharacterOffset() triggered.
        mController.showSelectionMenu(0, 0, 0, 0, 0, /* isEditable = */ true,
                /* isPasswordType = */ false, MOUNTAIN_FULL, /* selectionOffset = */ 0,
                /* canSelectAll = */ true,
                /* canRichlyEdit = */ true, /* shouldSuggest = */ true,
                MenuSourceType.MENU_SOURCE_ADJUST_SELECTION);

        order.verify(mView).startActionMode(
                isA(FloatingActionModeCallback.class), eq(ActionMode.TYPE_FLOATING));
        assertTrue(mController.isActionModeValid());
    }

    @Test
    @Feature({"TextInput", "SmartSelection"})
    public void testSmartSelectionLoggingExpansion() {
        InOrder order = inOrder(mLogger);
        SelectionClient.Result result = resultForAmphitheatre();

        // Setup SelectionClient for SelectionPopupController.
        TestSelectionClient client = new SelectionClientOnlyReturnTrue();
        client.setLogger(mLogger);
        client.setResult(result);
        client.setResultCallback(mController.getResultCallback());
        mController.setSelectionClient(client);

        // Long press triggered showSelectionMenu() call.
        mController.showSelectionMenu(0, 0, 0, 0, 0, /* isEditable = */ true,
                /* isPasswordType = */ false, AMPHITHEATRE, /* selectionOffset = */ 5,
                /* canSelectAll = */ true,
                /* canRichlyEdit = */ true, /* shouldSuggest = */ true,
                MenuSourceType.MENU_SOURCE_LONG_PRESS);

        when(mView.startActionMode(any(FloatingActionModeCallback.class), anyInt()))
                .thenReturn(mActionMode);

        order.verify(mLogger).logSelectionStarted(AMPHITHEATRE, 5, true);

        mController.getResultCallback().onClassified(result);

        // Call showSelectionMenu again, which is adjustSelectionByCharacterOffset triggered.
        mController.showSelectionMenu(0, 0, 0, 0, 0, /* isEditable = */ true,
                /* isPasswordType = */ false, AMPHITHEATRE_FULL, /* selectionOffset = */ 0,
                /* canSelectAll = */ true,
                /* canRichlyEdit = */ true, /* shouldSuggest = */ true,
                MenuSourceType.MENU_SOURCE_ADJUST_SELECTION);

        order.verify(mLogger).logSelectionModified(
                eq(AMPHITHEATRE_FULL), eq(0), isA(SelectionClient.Result.class));

        // Dragging selection handle, select "1600 Amphitheatre".
        mController.showSelectionMenu(0, 0, 0, 0, 0, /* isEditable = */ true,
                /* isPasswordType = */ false, AMPHITHEATRE_FULL.substring(0, 17),
                /* selectionOffset = */ 0,
                /* canSelectAll = */ true,
                /* canRichlyEdit = */ true, /* shouldSuggest = */ true,
                MenuSourceType.MENU_SOURCE_TOUCH_HANDLE);

        order.verify(mLogger, never())
                .logSelectionModified(anyString(), anyInt(), any(SelectionClient.Result.class));

        mController.getResultCallback().onClassified(resultForNoChange());

        order.verify(mLogger).logSelectionModified(
                eq("1600 Amphitheatre"), eq(0), isA(SelectionClient.Result.class));
    }

    @Test
    @Feature({"TextInput", "SmartSelection"})
    public void testSmartSelectionLoggingNoExpansion() {
        InOrder order = inOrder(mLogger);
        SelectionClient.Result result = resultForNoChange();

        // Setup SelectionClient for SelectionPopupController.
        TestSelectionClient client = new SelectionClientOnlyReturnTrue();
        client.setLogger(mLogger);
        client.setResult(result);
        client.setResultCallback(mController.getResultCallback());
        mController.setSelectionClient(client);

        // Long press triggered showSelectionMenu() call.
        mController.showSelectionMenu(0, 0, 0, 0, 0, /* isEditable = */ true,
                /* isPasswordType = */ false, AMPHITHEATRE, /* selectionOffset = */ 5,
                /* canSelectAll = */ true,
                /* canRichlyEdit = */ true, /* shouldSuggest = */ true,
                MenuSourceType.MENU_SOURCE_LONG_PRESS);

        when(mView.startActionMode(any(FloatingActionModeCallback.class), anyInt()))
                .thenReturn(mActionMode);
        order.verify(mLogger).logSelectionStarted(AMPHITHEATRE, 5, true);

        // No expansion.
        mController.getResultCallback().onClassified(result);
        order.verify(mLogger).logSelectionModified(
                eq(AMPHITHEATRE), eq(5), any(SelectionClient.Result.class));

        // Dragging selection handle, select "1600 Amphitheatre".
        mController.showSelectionMenu(0, 0, 0, 0, 0, /* isEditable = */ true,
                /* isPasswordType = */ false, AMPHITHEATRE_FULL.substring(0, 17),
                /* selectionOffset = */ 0,
                /* canSelectAll = */ true,
                /* canRichlyEdit = */ true, /* shouldSuggest = */ true,
                MenuSourceType.MENU_SOURCE_TOUCH_HANDLE);

        order.verify(mLogger, never())
                .logSelectionModified(anyString(), anyInt(), any(SelectionClient.Result.class));
        mController.getResultCallback().onClassified(resultForNoChange());
        order.verify(mLogger).logSelectionModified(
                eq("1600 Amphitheatre"), eq(0), isA(SelectionClient.Result.class));
    }

    @Test
    @Feature({"TextInput", "SmartSelection"})
    public void testBlockSelectionClientWhenUnprovisioned() {
        // Device is not provisioned.
        Settings.System.putInt(mContentResolver, Settings.Global.DEVICE_PROVISIONED, 0);

        TestSelectionClient client = Mockito.mock(TestSelectionClient.class);
        InOrder order = inOrder(mLogger, client);
        mController.setSelectionClient(client);

        // Long press triggered showSelectionMenu() call.
        mController.showSelectionMenu(0, 0, 0, 0, 0, /* isEditable = */ true,
                /* isPasswordType = */ false, AMPHITHEATRE, /* selectionOffset = */ 5,
                /* canSelectAll = */ true,
                /* canRichlyEdit = */ true, /* shouldSuggest = */ true,
                MenuSourceType.MENU_SOURCE_LONG_PRESS);
        order.verify(mLogger, never()).logSelectionStarted(anyString(), anyInt(), anyBoolean());
        order.verify(client, never()).requestSelectionPopupUpdates(anyBoolean());
    }

    @Test
    @Feature({"TextInput", "SmartSelection"})
    public void testBlockSelectionClientWhenIncognito() {
        // Incognito.
        when(mWebContents.isIncognito()).thenReturn(true);

        TestSelectionClient client = Mockito.mock(TestSelectionClient.class);
        InOrder order = inOrder(mLogger, client);
        mController.setSelectionClient(client);

        // Long press triggered showSelectionMenu() call.
        mController.showSelectionMenu(0, 0, 0, 0, 0, /* isEditable = */ true,
                /* isPasswordType = */ false, AMPHITHEATRE, /* selectionOffset = */ 5,
                /* canSelectAll = */ true,
                /* canRichlyEdit = */ true, /* shouldSuggest = */ true,
                MenuSourceType.MENU_SOURCE_LONG_PRESS);
        order.verify(mLogger, never()).logSelectionStarted(anyString(), anyInt(), anyBoolean());
        order.verify(client, never()).requestSelectionPopupUpdates(anyBoolean());
    }

    @Test
    @Feature({"TextInput", "HandleObserver"})
    public void testHandleObserverSelectionHandle() {
        SelectionInsertionHandleObserver handleObserver =
                Mockito.mock(SelectionInsertionHandleObserver.class);
        InOrder order = inOrder(handleObserver);
        mController.setSelectionInsertionHandleObserver(handleObserver);

        // Selection handles shown.
        mController.onSelectionEvent(SelectionEventType.SELECTION_HANDLES_SHOWN, 0, 0, 0, 0);

        // Selection handles drag started.
        mController.onDragUpdate(0.f, 0.f);
        order.verify(handleObserver).handleDragStartedOrMoved(0.f, 0.f);

        // Moving.
        mController.onDragUpdate(5.f, 5.f);
        order.verify(handleObserver).handleDragStartedOrMoved(5.f, 5.f);

        // Selection handle drag stopped.
        mController.onSelectionEvent(SelectionEventType.SELECTION_HANDLE_DRAG_STOPPED, 0, 0, 0, 0);
        order.verify(handleObserver).handleDragStopped();
    }

    @Test
    @Feature({"TextInput", "HandleObserver"})
    public void testHandleObserverInsertionHandle() {
        SelectionInsertionHandleObserver handleObserver =
                Mockito.mock(SelectionInsertionHandleObserver.class);
        InOrder order = inOrder(handleObserver);
        mController.setSelectionInsertionHandleObserver(handleObserver);

        // Insertion handle shown.
        mController.onSelectionEvent(SelectionEventType.INSERTION_HANDLE_SHOWN, 0, 0, 0, 0);

        // Insertion handle drag started.
        mController.onDragUpdate(0.f, 0.f);
        order.verify(handleObserver).handleDragStartedOrMoved(0.f, 0.f);

        // Moving.
        mController.onDragUpdate(5.f, 5.f);
        order.verify(handleObserver).handleDragStartedOrMoved(5.f, 5.f);

        // Insertion handle drag stopped.
        mController.onSelectionEvent(SelectionEventType.INSERTION_HANDLE_DRAG_STOPPED, 0, 0, 0, 0);
        order.verify(handleObserver).handleDragStopped();
    }

    // Result generated by long press "Amphitheatre" in "1600 Amphitheatre Parkway".
    private SelectionClient.Result resultForAmphitheatre() {
        SelectionClient.Result result = new SelectionClient.Result();
        result.startAdjust = -5;
        result.endAdjust = 8;
        result.label = "Maps";
        return result;
    }

    // Result generated by long press "Mountain" in "585 Franklin Street, Mountain View, CA 94041".
    private SelectionClient.Result resultForMountain() {
        SelectionClient.Result result = new SelectionClient.Result();
        result.startAdjust = -21;
        result.endAdjust = 15;
        result.label = "Maps";
        return result;
    }

    private SelectionClient.Result resultForNoChange() {
        SelectionClient.Result result = new SelectionClient.Result();
        result.startAdjust = 0;
        result.endAdjust = 0;
        result.label = "Maps";
        return result;
    }
}