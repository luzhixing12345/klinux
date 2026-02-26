// https://www.shadcn.io/icon/lucide-copy
const copyIcon = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="20" height="20" fill="none" stroke="#9198a1" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" style="opacity:1;"><rect width="14" height="14" x="8" y="8" rx="2" ry="2"/><path d="M4 16c-1.1 0-2-.9-2-2V4c0-1.1.9-2 2-2h10c1.1 0 2 .9 2 2"/></svg>';
// https://www.shadcn.io/icon/lucide-check
const copiedIcon = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="#3fb950" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" style="opacity:1;"><path fill="none"     d="M20 6L9 17l-5-5"/></svg>';
const copyRightOffset = 8;

function updateCopyButtonPosition(block) {
    var clip_board = block.querySelector('#code_copy');
    if (!clip_board) {
        return;
    }
    clip_board.style.right = `${copyRightOffset - block.scrollLeft}px`;
}

function add(block) {
    var clip_board = document.createElement('div');
    clip_board.id = 'code_copy';
    clip_board.innerHTML = copyIcon;
    clip_board.onclick = function () {
        clip_board.innerHTML = copiedIcon;
        var range = document.createRange();
        range.selectNodeContents(block.firstChild);
        var selection = window.getSelection();
        selection.removeAllRanges();
        selection.addRange(range);
        navigator.clipboard.writeText(block.firstChild.innerText);
        setTimeout(function () {
            if (document.body.contains(clip_board)) {
                clip_board.innerHTML = copyIcon;
            }
        }, 1200);
    }
    block.appendChild(clip_board)
    updateCopyButtonPosition(block);
}



function horizon_wheel(event, block, maxScroll) {
    event.preventDefault();
    const scrollAmount = event.deltaY * 0.5;
    const imageElement = block.querySelector('#code_copy');
    if (!imageElement) {
        return;
    }
    const computedStyle = window.getComputedStyle(imageElement);
    const currentRight = parseInt(computedStyle.getPropertyValue('right'));
    
    // 判断是否已滚动到左右边缘
    if (block.scrollLeft === 0 && scrollAmount < 0) {
        // 已经滚动到左边缘并且向左滚动,不做任何操作
    } else if (maxScroll-block.scrollLeft <= 50 && scrollAmount > 0) {
        // 已经滚动到右边缘并且向右滚动,不做任何操作
        // imageElement.style.right = `${currentRight - scrollAmount}px`;
    } else {
        imageElement.style.right = `${currentRight - scrollAmount}px`;
        // console.log(block.scrollLeft,maxScroll)
        block.scrollLeft += scrollAmount;
    }
}

function remove(block) {
    var clip_board = block.querySelector('#code_copy');
    if (clip_board) {
        block.removeChild(clip_board);
    }
}


function addCodeCopy() {
    var code_blocks = document.getElementsByTagName('pre')
    for (var i = 0; i < code_blocks.length; i++) {
        const code_block = code_blocks[i];
        code_block.addEventListener("mouseenter", () => add(code_block));
        code_block.addEventListener("mouseleave", () => remove(code_block));
        code_block.addEventListener("scroll", () => updateCopyButtonPosition(code_block));
    }
}
addCodeCopy()
